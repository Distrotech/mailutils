/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001, 2005, 2006, 2007 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
   MA 02110-1301 USA */

#include "imap4d.h"

/* APPEND mbox [(flags)] [date_time] message_literal */
int
imap4d_append (struct imap4d_command *command, char *arg)
{
  char *sp;
  char *mboxname;
  int flags = 0;
  mu_mailbox_t dest_mbox = NULL;
  int status;

  mboxname = util_getword (arg, &sp);
  if (!mboxname)
    return util_finish (command, RESP_BAD, "Too few arguments");

  util_unquote (&mboxname);
  
  if (*sp == '(' && util_parse_attributes (sp+1, &sp, &flags))
    return util_finish (command, RESP_BAD, "Missing closing parenthesis");
  
  mboxname = namespace_getfullpath (mboxname, "/");
  if (!mboxname)
    return util_finish (command, RESP_NO, "Couldn't open mailbox"); 

  status = mu_mailbox_create_default (&dest_mbox, mboxname);
  if (status == 0)
    {
      /* It SHOULD NOT automatifcllly create the mailbox. */
      status = mu_mailbox_open (dest_mbox, MU_STREAM_RDWR);
      if (status == 0)
	{
	  status = imap4d_append0 (dest_mbox, flags, sp);
	  mu_mailbox_close (dest_mbox);
	}
      mu_mailbox_destroy (&dest_mbox);
    }
  
  free (mboxname);
  if (status == 0)
    return util_finish (command, RESP_OK, "Completed");

  return util_finish (command, RESP_NO, "[TRYCREATE] failed");
}

static int
_append_date (mu_envelope_t envelope, char *buf, size_t len, size_t *pnwrite)
{
  mu_message_t msg = mu_envelope_get_owner (envelope);
  size_t size;
  if (!buf)
    size = MU_ENVELOPE_DATE_LENGTH;
  else
    {
      struct tm **tm = mu_message_get_owner (msg);
      size = mu_strftime (buf, len, "%a %b %d %H:%M:%S %Y", *tm);
    }
  if (pnwrite)
    *pnwrite = size;
  return 0;
}

static int
_append_sender (mu_envelope_t envelope, char *buf, size_t len, size_t *pnwrite)
{
  size_t n = mu_cpystr (buf, "GNU-imap4d", len);
  if (pnwrite)
    *pnwrite = n;
  return 0;
}

int
imap4d_append0 (mu_mailbox_t mbox, int flags, char *text)
{
  mu_stream_t stream;
  int rc = 0;
  size_t len = 0;
  mu_message_t msg = 0;
  struct tm *tm;
  time_t t;
  mu_envelope_t env;
    
  if (mu_message_create (&msg, &tm))
    return 1;
  
  if (mu_memory_stream_create (&stream, 0, MU_STREAM_RDWR)
      || mu_stream_open (stream))
    {
      mu_message_destroy (&msg, &tm);
      return 1;
    }

  while (*text && isspace (*text))
    text++;

  /* If a date_time is specified, the internal date SHOULD be set in the
     resulting message; otherwise, the internal date of the resulting
     message is set to the current date and time by default. */
  if (util_parse_internal_date0 (text, &t, &text) == 0)
    {
      while (*text && isspace(*text))
	text++;
    }
  else
    {
      time(&t);
    }
  tm = gmtime(&t);

  mu_stream_write (stream, text, strlen (text), len, &len);
  mu_message_set_stream (msg, stream, &tm);

  mu_envelope_create (&env, msg);
  mu_envelope_set_date (env, _append_date, msg);
  mu_envelope_set_sender (env, _append_sender, msg);
  mu_message_set_envelope (msg, env, &tm);
  rc = mu_mailbox_append_message (mbox, msg);
  if (rc == 0 && flags)
    {
      size_t num = 0;
      mu_attribute_t attr = NULL;
      mu_mailbox_messages_count (mbox, &num);
      mu_mailbox_get_message (mbox, num, &msg);
      mu_message_get_attribute (msg, &attr);
      mu_attribute_set_flags (attr, flags);
    }

  mu_message_destroy (&msg, &tm);
  return rc;
}


