/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA  */

#include "imap4d.h"

static int select_flags;

/* select          ::= "SELECT" SPACE mailbox  */

int
imap4d_select (struct imap4d_command *command, char *arg)
{
  return imap4d_select0 (command, arg, MU_STREAM_RDWR);
}

/* This code is share with EXAMINE.  */
int
imap4d_select0 (struct imap4d_command *command, char *arg, int flags)
{
  char *mailbox_name, *sp = NULL;
  int status;

  /* FIXME: Check state.  */

  mailbox_name = util_getword (arg, &sp);
  util_unquote (&mailbox_name);
  if (mailbox_name == NULL || *mailbox_name == '\0')
    return util_finish (command, RESP_BAD, "Too few arguments");

  /* Even if a mailbox is selected, a SELECT EXAMINE or LOGOUT
     command MAY be issued without previously issuing a CLOSE command.
     The SELECT, EXAMINE, and LOGUT commands implictly close the
     currently selected mailbox without doing an expunge.  */
  if (mbox)
    {
      mailbox_save_attributes (mbox);
      mailbox_close (mbox);
      mailbox_destroy (&mbox);
      /* Destroy the old uid table.  */
      imap4d_sync ();
    }

  mailbox_name = namespace_getfullpath (mailbox_name, "/");

  if (!mailbox_name)
    return util_finish (command, RESP_NO, "Couldn't open mailbox");

  if ((status = mailbox_create (&mbox, mailbox_name)) == 0
      && (status = mailbox_open (mbox, flags)) == 0)
    {
      select_flags = flags;
      state = STATE_SEL;
      if ((status = imap4d_select_status ()) == 0)
	{
	  free (mailbox_name);
	  /* Need to set the state explicitely for select.  */
	  return util_send ("%s OK [%s] %s Completed\r\n", command->tag,
			    (MU_STREAM_READ == flags) ?
			    "READ-ONLY" : "READ-WRITE", command->name);
	}
    }
  status = util_finish (command, RESP_NO, "Couldn't open %s, %s",
			mailbox_name, mu_strerror (status));
  free (mailbox_name);
  return status;
}

/* The code is shared between select and noop */
int
imap4d_select_status ()
{
  const char *mflags = "\\Answered \\Flagged \\Deleted \\Seen \\Draft";
  const char *pflags = "\\Answered \\Deleted \\Seen";
  unsigned long uidvalidity = 0;
  size_t count = 0, recent = 0, unseen = 0, uidnext = 0;
  int status = 0;

  if (state != STATE_SEL)
    return 0; /* FIXME: this should be something! */

  if ((status = util_uidvalidity (mbox, &uidvalidity))
      || (status = mailbox_uidnext (mbox, &uidnext))
      || (status = mailbox_messages_count (mbox, &count))
      || (status = mailbox_messages_recent (mbox, &recent))
      || (status = mailbox_message_unseen (mbox, &unseen)))
    return status;

  /* This outputs EXISTS and RECENT responses */
  imap4d_sync();
  util_out (RESP_OK, "[UIDVALIDITY %d] UID valididy status", uidvalidity);
  util_out (RESP_OK, "[UIDNEXT %d] Predicted next uid", uidnext);
  if (unseen)
    util_out (RESP_OK, "[UNSEEN %d] first unseen messsage ", unseen);
  util_out (RESP_NONE, "FLAGS (%s)", mflags);
  /* FIXME:
     - '\*' can be supported if we use the attribute_set userflag()
     - Answered is still not set in the mailbox code.  */
  if (select_flags & MU_STREAM_READ)
    util_out (RESP_OK, "[PERMANENTFLAGS ()] No Permanent flags");
  else
    util_out (RESP_OK, "[PERMANENTFLAGS (%s)] Permanent flags", pflags);

  return 0;
}

