/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2001 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#include "imap4d.h"

/* select          ::= "SELECT" SPACE mailbox  */

int
imap4d_select (struct imap4d_command *command, char *arg)
{
  if (! (command->states & state))
    return util_finish (command, RESP_BAD, "Wrong state");
  return imap4d_select0 (command, arg, MU_STREAM_RDWR);
}

/* This code is share with EXAMINE.  */
int
imap4d_select0 (struct imap4d_command *command, char *arg, int flags)
{
  char *mailbox_name, *sp = NULL;
  int status;
  struct passwd *pw;

  /* FIXME: Check state.  */

  mailbox_name = util_getword (arg, &sp);
  util_unquote (&mailbox_name);
  if (mailbox_name == NULL || *mailbox_name == '\0')
    return util_finish (command, RESP_BAD, "Too few arguments");

  /* Even if a mailbox is slected, a SLECT EXAMINE or LOGOUT
     command MAY be issued without previously issuing a CLOSE command.
     The SELECT, EXAMINE, and LOGUT commands implictly close the
     currently selected mailbox withut doing an expunge.  */
  if (mbox)
    {
      mailbox_close (mbox);
      mailbox_destroy (&mbox);
      /* Destroy the old uid table.  */
      imap4d_sync ();
    }

  if (strcasecmp (mailbox_name, "INBOX") == 0)
    {
      pw = getpwuid (getuid ());
      if (pw)
	{
	  mailbox_name  = calloc (strlen (_PATH_MAILDIR) + 1
				  + strlen (pw->pw_name) + 1, 1);
	  sprintf (mailbox_name, "%s/%s", _PATH_MAILDIR, pw->pw_name);
	}
      else
	mailbox_name = strdup ("/dev/null");
    }
  else
    mailbox_name = util_getfullpath (mailbox_name, "/");

  if (mailbox_create (&mbox, mailbox_name) == 0
      && mailbox_open (mbox, flags) == 0)
    {
      const char *mflags = "\\Answered \\Flagged \\Deleted \\Seen \\Draft";
      const char *pflags = "\\Answered \\Deleted \\Seen";
      unsigned long uidvalidity = 0;
      size_t count = 0, recent = 0, unseen = 0, uidnext = 0;

      free (mailbox_name);

      mailbox_uidvalidity (mbox, &uidvalidity);
      mailbox_uidnext (mbox, &uidnext);
      mailbox_messages_count (mbox, &count);
      mailbox_messages_recent (mbox, &recent);
      mailbox_message_unseen (mbox, &unseen);
      util_out (RESP_NONE, "%d EXISTS", count);
      util_out (RESP_NONE, "%d RECENT", recent);
      util_out (RESP_OK, "[UIDVALIDITY (%d)] UID valididy status",
		uidvalidity);
      util_out (RESP_OK, "[UIDNEXT %d] Predicted next uid", uidnext);
      if (unseen)
	util_out (RESP_OK, "[UNSEEN (%d)] first unseen messsage ", unseen);
      util_out (RESP_NONE, "FLAGS (%s)", mflags);
      /* FIXME:
	 - '\*' can be supported if we use the attribute_set userflag()
	 - Answered is still not set in the mailbox code.  */
      if (flags == MU_STREAM_READ)
	util_out (RESP_OK, "[PERMANENTFLAGS ()] No Permanent flags");
      else
	util_out (RESP_OK, "[PERMANENTFLAGS (%s)] Permanent flags", pflags);
      /* Need to set the state explicitely for select.  */
      state = STATE_SEL;
      return util_send ("%s OK [%s] %s Completed\r\n", command->tag,
			(MU_STREAM_READ == flags) ?
			"READ-ONLY" : "READ-WRITE", command->name);
    }
  status = util_finish (command, RESP_NO, "Couldn't open %s", mailbox_name);
  free (mailbox_name);
  return status;
}
