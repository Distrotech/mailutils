/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001, 2005-2012 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>. */

#include "imap4d.h"

struct _temp_envelope
{
  struct tm tm;
  struct mu_timezone tz;
  char *sender;
};

static int
_temp_envelope_date (mu_envelope_t envelope, char *buf, size_t len,
		     size_t *pnwrite)
{
  struct _temp_envelope *tenv = mu_envelope_get_owner (envelope);
  int rc;
  mu_stream_t str;
  mu_stream_stat_buffer stat;
  
  if (!buf)
    {
      if (!pnwrite)
	return MU_ERR_OUT_PTR_NULL;
      
      rc = mu_nullstream_create (&str, MU_STREAM_WRITE);
    }
  else
    rc = mu_fixed_memory_stream_create (&str, buf, len, MU_STREAM_WRITE);

  if (rc)
    return rc;
  mu_stream_set_stat (str, MU_STREAM_STAT_MASK (MU_STREAM_STAT_OUT), stat);
      
  rc = mu_c_streamftime (str, MU_DATETIME_FROM, &tenv->tm, &tenv->tz);
  if (rc == 0)
    {
      mu_stream_flush (str);
      if (pnwrite)
	*pnwrite = stat[MU_STREAM_STAT_OUT];
      rc = mu_stream_write (str, "", 1, NULL);
    }
  mu_stream_unref (str);
  
  if (rc)
    return rc;
  return 0;
}

static int
_temp_envelope_sender (mu_envelope_t envelope, char *buf, size_t len,
		       size_t *pnwrite)
{
  struct _temp_envelope *tenv = mu_envelope_get_owner (envelope);
  size_t n = mu_cpystr (buf, tenv->sender, len);
  if (pnwrite)
    *pnwrite = n;       
  return 0;
}

static int
_temp_envelope_destroy (mu_envelope_t envelope)
{
  struct _temp_envelope *tenv = mu_envelope_get_owner (envelope);
  free (tenv->sender);
  return 0;
}

int
imap4d_append0 (mu_mailbox_t mbox, int flags, char *date_time, char *text,
		char **err_text)
{
  mu_stream_t stream;
  int rc = 0;
  mu_message_t msg = 0;
  mu_envelope_t env = NULL;
  size_t size;
  struct _temp_envelope tenv;

  memset (&tenv, 0, sizeof (tenv));
	  
  text = mu_str_skip_class (text, MU_CTYPE_BLANK);

  size = strlen (text);
  rc = quota_check (size);
  if (rc != RESP_OK)
    {
      *err_text = rc == RESP_NO ?
	                   "Mailbox quota exceeded" : "Operation failed";
      return 1;
    }

  /* If a date_time is specified, the internal date SHOULD be set in the
     resulting message; otherwise, the internal date of the resulting
     message is set to the current date and time by default. */
  if (date_time)
    {
      if (mu_scan_datetime (date_time, MU_DATETIME_INTERNALDATE, &tenv.tm,
			    &tenv.tz, NULL))
	{
	  *err_text = "Invalid date/time format";
	  return 1;
	}
      rc = mu_envelope_create (&env, &tenv);
      if (rc)
	return rc;
      mu_envelope_set_date (env, _temp_envelope_date, &tenv);
      mu_envelope_set_sender (env, _temp_envelope_sender, &tenv);
      mu_envelope_set_destroy (env, _temp_envelope_destroy, &tenv);
    }

  if (mu_static_memory_stream_create (&stream, text, size))
    {
      if (env)
	mu_envelope_destroy (&env, mu_envelope_get_owner (env));
      return 1;
    }
  
  rc = mu_message_from_stream_with_envelope (&msg, stream, env);
  mu_stream_unref (stream);

  if (rc)
    {
      if (env)
	mu_envelope_destroy (&env, mu_envelope_get_owner (env));
      return 1;
    }

  if (env)
    {
      /* Restore sender */
      mu_header_t hdr = NULL;
      char *val;
      
      mu_message_get_header (msg, &hdr);
      if (mu_header_aget_value_unfold (hdr, MU_HEADER_ENV_SENDER, &val) == 0 ||
	  mu_header_aget_value_unfold (hdr, MU_HEADER_SENDER, &val) == 0 ||
	  mu_header_aget_value_unfold (hdr, MU_HEADER_FROM, &val) == 0)
	{
	  mu_address_t addr;
	  rc = mu_address_create (&addr, val);
	  free (val);
	  if (rc == 0)
	    {
	      mu_address_aget_email (addr, 1, &tenv.sender);
	      mu_address_destroy (&addr);
	    }
	}

      if (!tenv.sender)
	tenv.sender = mu_strdup ("GNU-imap4d");
    }
      
  imap4d_enter_critical ();
  rc = mu_mailbox_append_message (mbox, msg);
  if (rc == 0)
    {
      if (flags)
	{
	  size_t num = 0;
	  mu_attribute_t attr = NULL;
	  mu_message_t temp;
	  
	  mu_mailbox_messages_count (mbox, &num);
	  mu_mailbox_get_message (mbox, num, &temp);
	  mu_message_get_attribute (temp, &attr);
	  mu_attribute_set_flags (attr, flags);
	}
      /* FIXME: If not INBOX */
      quota_update (size);
    }
  imap4d_leave_critical ();
  
  mu_message_unref (msg);
  if (env)
    mu_envelope_destroy (&env, mu_envelope_get_owner (env));

  return rc;
}


/* APPEND mbox [(flags)] [date_time] message_literal */
int
imap4d_append (struct imap4d_session *session,
               struct imap4d_command *command, imap4d_tokbuf_t tok)
{
  int i;
  char *mboxname;
  int flags = 0;
  mu_mailbox_t dest_mbox = NULL;
  int status;
  int argc = imap4d_tokbuf_argc (tok);
  char *date_time;
  char *msg_text;
  char *err_text = "[TRYCREATE] failed";
  
  if (argc < 4)
    return io_completion_response (command, RESP_BAD, "Too few arguments");
      
  mboxname = imap4d_tokbuf_getarg (tok, IMAP4_ARG_1);
  if (!mboxname)
    return io_completion_response (command, RESP_BAD, "Too few arguments");

  i = IMAP4_ARG_2;
  if (imap4d_tokbuf_getarg (tok, i)[0] == '(')
    {
      while (++i < argc)
	{
	  char *arg = imap4d_tokbuf_getarg (tok, i);
	  
	  if (arg[0] == ')')
	    break;
	  if (mu_imap_flag_to_attribute (arg, &flags))
	    return io_completion_response (command, RESP_BAD,
					   "Unrecognized flag");
	}
      if (i == argc)
	return io_completion_response (command, RESP_BAD, 
	                               "Missing closing parenthesis");
      i++;
    }

  switch (argc - i)
    {
    case 2:
      /* Date/time is present */
      date_time = imap4d_tokbuf_getarg (tok, i);
      i++;
      break;

    case 1:
      date_time = NULL;
      break;

    default:
      return io_completion_response (command, RESP_BAD, "Too many arguments");
    }

  msg_text = imap4d_tokbuf_getarg (tok, i);
  
  mboxname = namespace_getfullpath (mboxname, NULL);
  if (!mboxname)
    return io_completion_response (command, RESP_NO, "Couldn't open mailbox"); 

  status = mu_mailbox_create_default (&dest_mbox, mboxname);
  if (status == 0)
    {
      /* It SHOULD NOT automatically create the mailbox. */
      status = mu_mailbox_open (dest_mbox, MU_STREAM_RDWR);
      if (status == 0)
	{
	  status = imap4d_append0 (dest_mbox, flags, date_time, msg_text,
				   &err_text);
	  mu_mailbox_close (dest_mbox);
	}
      mu_mailbox_destroy (&dest_mbox);
    }
  
  free (mboxname);
  if (status == 0)
    return io_completion_response (command, RESP_OK, "Completed");

  return io_completion_response (command, RESP_NO, "%s", err_text);
}


