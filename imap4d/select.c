/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001, 2003, 2005-2012 Free Software Foundation,
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

static int select_flags;

/* select          ::= "SELECT" SPACE mailbox  */

int
imap4d_select (struct imap4d_session *session,
               struct imap4d_command *command, imap4d_tokbuf_t tok)
{
  if (imap4d_tokbuf_argc (tok) != 3)
    return io_completion_response (command, RESP_BAD, "Invalid arguments");
  return imap4d_select0 (command, imap4d_tokbuf_getarg (tok, IMAP4_ARG_1),
			 MU_STREAM_RDWR);
}

/* This code is shared with EXAMINE.  */
int
imap4d_select0 (struct imap4d_command *command, const char *mboxname,
		int flags)
{
  int status;
  char *mailbox_name;
  
  /* FIXME: Check state.  */

  /* Even if a mailbox is selected, a SELECT EXAMINE or LOGOUT
     command MAY be issued without previously issuing a CLOSE command.
     The SELECT, EXAMINE, and LOGUT commands implictly close the
     currently selected mailbox without doing an expunge.  */
  if (mbox)
    {
      imap4d_enter_critical ();
      mu_mailbox_sync (mbox);
      mu_mailbox_close (mbox);
      manlock_unlock (mbox);
      imap4d_leave_critical ();
      mu_mailbox_destroy (&mbox);
      /* Destroy the old uid table.  */
      imap4d_sync ();
    }

  if (strcmp (mboxname, "INBOX") == 0)
    flags |= MU_STREAM_CREAT;
  mailbox_name = namespace_getfullpath (mboxname, NULL);

  if (!mailbox_name)
    return io_completion_response (command, RESP_NO, "Couldn't open mailbox");

  if (flags & MU_STREAM_RDWR)
    {
      status = manlock_open_mailbox (&mbox, mailbox_name, 1, flags);
    }
  else
    {
      status = mu_mailbox_create_default (&mbox, mailbox_name);
      if (status)
	mu_diag_funcall (MU_DIAG_ERROR, "mu_mailbox_create_default",
			 mailbox_name,
			 status);
      else
	{
	  status = mu_mailbox_open (mbox, flags);
	  if (status)
	    {
	      mu_diag_funcall (MU_DIAG_ERROR, "mu_mailbox_open",
			       mailbox_name,
			       status);
	      mu_mailbox_destroy (&mbox);
	    }
	}
    }
  
  if (status == 0)
    {
      select_flags = flags;
      state = STATE_SEL;

      imap4d_set_observer (mbox);
      
      if ((status = imap4d_select_status ()) == 0)
	{
	  free (mailbox_name);
	  /* Need to set the state explicitely for select.  */
	  return io_sendf ("%s OK [%s] %s Completed\n", command->tag,
			   ((flags & MU_STREAM_RDWR) == MU_STREAM_RDWR) ?
			   "READ-WRITE" : "READ-ONLY", command->name);
	}
    }
  
  mu_mailbox_destroy (&mbox);
  status = io_completion_response (command, RESP_NO, "Could not open %s: %s",
			mboxname, mu_strerror (status));
  free (mailbox_name);
  return status;
}

/* The code is shared between select and noop */
int
imap4d_select_status ()
{
  const char *mflags = "\\Answered \\Flagged \\Deleted \\Seen \\Draft";
  unsigned long uidvalidity = 0;
  size_t count = 0, recent = 0, unseen = 0, uidnext = 0;
  int status = 0;

  if (state != STATE_SEL)
    return 0; /* FIXME: this should be something! */

  if ((status = util_uidvalidity (mbox, &uidvalidity))
      || (status = mu_mailbox_uidnext (mbox, &uidnext))
      || (status = mu_mailbox_messages_count (mbox, &count))
      || (status = mu_mailbox_messages_recent (mbox, &recent))
      || (status = mu_mailbox_message_unseen (mbox, &unseen)))
    return status;

  /* This outputs EXISTS and RECENT responses */
  imap4d_sync();
  io_untagged_response (RESP_OK, "[UIDVALIDITY %lu] UID valididy status", 
                           uidvalidity);
  io_untagged_response (RESP_OK, "[UIDNEXT %lu] Predicted next uid",
	                   (unsigned long) uidnext);
  if (unseen)
    io_untagged_response (RESP_OK, "[UNSEEN %lu] first unseen message",
	                     (unsigned long) unseen);
  io_untagged_response (RESP_NONE, "FLAGS (%s)", mflags);
  /* FIXME:
     - '\*' can be supported if we use the attribute_set userflag()
     - Answered is still not set in the mailbox code.  */
  if (!(select_flags & MU_STREAM_WRITE))
    io_untagged_response (RESP_OK, "[PERMANENTFLAGS ()] No permanent flags");
  else
    io_untagged_response (RESP_OK, "[PERMANENTFLAGS (%s)] Permanent flags",
                          mflags);

  return 0;
}

