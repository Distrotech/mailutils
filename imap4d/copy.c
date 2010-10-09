/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001, 2005, 2007, 2008, 2009, 2010 Free Software
   Foundation, Inc.

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

/*
6.4.7.  COPY Command

   Arguments:  message set
               mailbox name

   Responses:  no specific responses for this command

   Result:     OK - copy completed
               NO - copy error: can't copy those messages or to that
                    name
               BAD - command unknown or arguments invalid

  copy messages in argv[2] to mailbox in argv[3]
 */

int
imap4d_copy (struct imap4d_command *command, imap4d_tokbuf_t tok)
{
  int rc;
  char *text;

  if (imap4d_tokbuf_argc (tok) != 4)
    return io_completion_response (command, RESP_BAD, "Invalid arguments");
  
  rc = imap4d_copy0 (tok, 0, &text);
  
  if (rc == RESP_NONE)
    {
      /* Reset the state ourself.  */
      int new_state = (rc == RESP_OK) ? command->success : command->failure;
      if (new_state != STATE_NONE)
	state = new_state;
      return io_sendf ("%s %s\n", command->tag, text);
    }
  return io_completion_response (command, rc, "%s", text);
}

static int
copy_check_size (mu_mailbox_t mbox, size_t n, size_t *set, mu_off_t *size)
{
  int status;
  size_t i;
  mu_off_t total = 0;
  
  for (i = 0; i < n; i++)
    {
      mu_message_t msg = NULL;
      size_t msgno = set[i];
      if (msgno)
	{
	  status = mu_mailbox_get_message (mbox, msgno, &msg);
	  if (status)
	    {
	      mu_diag_funcall (MU_DIAG_ERROR, "mu_mailbox_get_message", NULL,
			       status);
	      return RESP_BAD;
	    }
	  else 
	    {
	      size_t size;
	      status = mu_message_size (msg, &size);
	      if (status)
		{
		  mu_diag_funcall (MU_DIAG_ERROR, "mu_message_size", NULL,
				   status);
		  return RESP_BAD;
		}
	      total += size;
	    }
	}
    }
  *size = total;
  return quota_check (total);
}

static int
try_copy (mu_mailbox_t dst, mu_mailbox_t src, size_t n, size_t *set)
{
  int result;
  size_t i;
  mu_off_t total;
  
  result = copy_check_size (src, n, set, &total);
  if (result)
    return result;
  
  for (i = 0; i < n; i++)
    {
      mu_message_t msg = NULL;
      size_t msgno = set[i];

      if (msgno)
	{
	  int status = mu_mailbox_get_message (src, msgno, &msg);
	  if (status)
	    {
	      mu_diag_funcall (MU_DIAG_ERROR, "mu_mailbox_get_message", NULL,
			       status);
	      return RESP_BAD;
	    }

	  status = mu_mailbox_append_message (dst, msg);
	  if (status)
	    {
	      mu_diag_funcall (MU_DIAG_ERROR, "mu_mailbox_append_message",
			       NULL,
			       status);
	      return RESP_BAD;
	    }
	}
    }
  quota_update (total);
  return RESP_OK;
}

static int
safe_copy (mu_mailbox_t dst, mu_mailbox_t src, size_t n, size_t *set,
	   char **err_text)
{
  size_t nmesg;
  int status;
  
  status = mu_mailbox_messages_count (dst, &nmesg);
  if (status)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_mailbox_messages_count",
		       NULL, status);
      *err_text = "Operation failed";
      return RESP_NO;
    }

  status = try_copy (dst, src, n, set);
  if (status)
    {
      size_t maxmesg;

      if (status == RESP_NO)
	*err_text = "Mailbox quota exceeded";
      else
	*err_text = "Operation failed";
      
      /* If the COPY command is unsuccessful for any reason, server
	 implementations MUST restore the destination mailbox to its state
	 before the COPY attempt. */

      status = mu_mailbox_messages_count (dst, &maxmesg);
      if (status)
	{
	  mu_url_t url = NULL;

	  mu_mailbox_get_url (dst, &url);
	  mu_error (_("cannot count messages in mailbox %s: %s"),
		    mu_url_to_string (url), mu_strerror (status));
	  imap4d_bye (ERR_MAILBOX_CORRUPTED);
	}
      
      for (nmesg++; nmesg <= maxmesg; nmesg++)
	{
	  mu_message_t msg;
	  
	  if (mu_mailbox_get_message (dst, nmesg, &msg) == 0)
	    {
	      mu_attribute_t attr;
	      mu_message_get_attribute (msg, &attr);
	      mu_attribute_set_userflag (attr, MU_ATTRIBUTE_DELETED);
	    }
	}
      
      status = mu_mailbox_flush (dst, 1);
      if (status)
	{
	  mu_url_t url = NULL;

	  mu_mailbox_get_url (dst, &url);
	  mu_error (_("cannot flush mailbox %s: %s"),
		    mu_url_to_string (url), mu_strerror (status));
	  imap4d_bye (ERR_MAILBOX_CORRUPTED);
	}
      return RESP_NO;
    }
  
  return RESP_OK;
}

int
imap4d_copy0 (imap4d_tokbuf_t tok, int isuid, char **err_text)
{
  int status;
  char *msgset;
  char *name;
  char *mailbox_name;
  const char *delim = "/";
  size_t *set = NULL;
  int n = 0;
  mu_mailbox_t cmbox = NULL;
  int arg = IMAP4_ARG_1 + !!isuid;
  int ns;

  *err_text = NULL;
  if (imap4d_tokbuf_argc (tok) != arg + 2)
    {
      *err_text = "Invalid arguments";
      return 1;
    }
  
  msgset = imap4d_tokbuf_getarg (tok, arg);
  name = imap4d_tokbuf_getarg (tok, arg + 1);
  /* Get the message numbers in set[].  */
  status = util_msgset (msgset, &set, &n, isuid);
  if (status != 0)
    {
      /* See RFC 3501, section 6.4.8, and a comment to the equivalent code
	 in fetch.c */
      *err_text = "Completed";
      return RESP_OK;
    }

  if (isuid)
    {
      int i;
      /* Fixup the message set. Perhaps util_msgset should do it itself? */
      for (i = 0; i < n; i++)
	set[i] = uid_to_msgno (set[i]);
    }
  
  mailbox_name = namespace_getfullpath (name, delim, &ns);

  if (!mailbox_name)
    {
      *err_text = "Copy failed";
      return RESP_NO;
    }

  /* If the destination mailbox does not exist, a server should return
     an error.  */
  status = mu_mailbox_create_default (&cmbox, mailbox_name);
  if (status == 0)
    {
      /* It SHOULD NOT automatifcllly create the mailbox. */
      status = mu_mailbox_open (cmbox, MU_STREAM_RDWR | mailbox_mode[ns]);
      if (status == 0)
	{
	  status = safe_copy (cmbox, mbox, n, set, err_text);
	  mu_mailbox_close (cmbox);
	}
      mu_mailbox_destroy (&cmbox);
    }
  free (set);
  free (mailbox_name);

  if (status == 0)
    {
      *err_text = "Completed";
      return RESP_OK;
    }

  /* Unless it is certain that the destination mailbox cannot be created,
     the server MUST send the response code "[TRYCREATE]" as the prefix
     of the text of the tagged NO response.  This gives a hint to the
     client that it can attempt a CREATE command and retry the copy if
     the CREATE is successful.  */
  if (!*err_text)
    *err_text = "[TRYCREATE] failed";
  return RESP_NO;
}
