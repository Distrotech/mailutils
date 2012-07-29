/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001, 2005, 2007-2012 Free Software Foundation,
   Inc.

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
imap4d_copy (struct imap4d_session *session,
             struct imap4d_command *command, imap4d_tokbuf_t tok)
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

struct copy_env
{
  mu_mailbox_t dst;
  mu_off_t total;
  int ret;
  char **err_text;
};

static int
size_sum (size_t msgno, mu_message_t msg, void *data)
{
  struct copy_env *env = data;
  int rc;
  
  size_t size;
  rc = mu_message_size (msg, &size);
  if (rc)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_message_size", NULL, rc);
      env->ret = RESP_BAD;
      return MU_ERR_FAILURE;
    }
  env->total += size;
  return 0;
}

static int
do_copy (size_t msgno, mu_message_t msg, void *data)
{
  struct copy_env *env = data;
  int status;

  imap4d_enter_critical ();
  status = mu_mailbox_append_message (env->dst, msg);
  imap4d_leave_critical ();
  if (status)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_mailbox_append_message", NULL,
		       status);
      env->ret = RESP_BAD;
      return MU_ERR_FAILURE;
    }

  return 0;
}

static int
try_copy (mu_mailbox_t dst, mu_msgset_t msgset, char **err_text)
{
  int rc;
  struct copy_env env;

  env.dst = dst;
  env.total = 0;
  env.ret = RESP_OK;
  env.err_text = err_text;

  *env.err_text = "Operation failed";

  /* Check size */
  rc = mu_msgset_foreach_message (msgset, size_sum, &env);
  if (rc)
    return RESP_NO;
  if (env.ret != RESP_OK)
    return env.ret;
  rc = quota_check (env.total);
  if (rc)
    {
      *env.err_text = "Mailbox quota exceeded";
      return RESP_NO;
    }
  env.total = 0;
  rc = mu_msgset_foreach_message (msgset, do_copy, &env);
  quota_update (env.total);
  if (rc)
    return RESP_NO;
  return env.ret;
}
  
static int
safe_copy (mu_mailbox_t dst, mu_msgset_t msgset, char **err_text)
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

  status = try_copy (dst, msgset, err_text);
  if (status != RESP_OK)
    {
      size_t maxmesg;

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

      imap4d_enter_critical ();
      status = mu_mailbox_flush (dst, 1);
      imap4d_leave_critical ();
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
  char *msgset_str;
  mu_msgset_t msgset;
  char *name;
  char *mailbox_name;
  char *end;
  mu_mailbox_t cmbox = NULL;
  int arg = IMAP4_ARG_1 + !!isuid;
  int ns;
  
  *err_text = NULL;
  if (imap4d_tokbuf_argc (tok) != arg + 2)
    {
      *err_text = "Invalid arguments";
      return 1;
    }
  
  msgset_str = imap4d_tokbuf_getarg (tok, arg);
  name = imap4d_tokbuf_getarg (tok, arg + 1);
  status = mu_msgset_create (&msgset, mbox, MU_MSGSET_NUM);
  if (status)
    {
      *err_text = "Software error";
      return RESP_BAD;
    }
    
  status = mu_msgset_parse_imap (msgset,
				 isuid ? MU_MSGSET_UID : MU_MSGSET_NUM,
				 msgset_str, &end);
  if (status)
    {
      mu_msgset_free (msgset);
      *err_text = "Error parsing message set";
      /* FIXME: print error location */
      return RESP_BAD;
    }

  mailbox_name = namespace_getfullpath (name, &ns);

  if (!mailbox_name)
    {
      mu_msgset_free (msgset);
      *err_text = "Copy failed";
      return RESP_NO;
    }

  /* If the destination mailbox does not exist, a server should return
     an error. */
  status = mu_mailbox_create_default (&cmbox, mailbox_name);
  if (status == 0)
    {
      /* It SHOULD NOT automatifcllly create the mailbox. */
      status = mu_mailbox_open (cmbox, MU_STREAM_RDWR | mailbox_mode[ns]);
      if (status == 0)
	{
	  mu_list_t msglist;
	  mu_msgset_get_list (msgset, &msglist);
	  if (!mu_list_is_empty (msglist))
	    status = safe_copy (cmbox, msgset, err_text);
	  mu_mailbox_close (cmbox);
	}
      mu_mailbox_destroy (&cmbox);
    }
  mu_msgset_free (msgset);

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
