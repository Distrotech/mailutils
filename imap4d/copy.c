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

/*
 * copy messages in argv[2] to mailbox in argv[3]
 */

/* FIXME if the mailbox is the one selecte we should send notif.  */
int
imap4d_copy (struct imap4d_command *command, char *arg)
{
  int status;
  char *msgset;
  char *name;
  char *mailbox_name;
  const char *delim = "/";
  char *sp = NULL;
  int *set = NULL;
  size_t n = 0;
  mailbox_t cmbox = NULL;

  if (! (command->states & state))
    return util_finish (command, RESP_BAD, "Wrong state");

  msgset = util_getword (arg, &sp);
  name = util_getword (NULL, &sp);

  util_unquote (&name);
  if (!msgset || !name || *name == '\0')
    return util_finish (command, RESP_BAD, "Too few args");

  /* Get the message numbers in set[].  */
  status = util_msgset (msgset, &set, &n, 0);
  if (status != 0)
    return util_finish (command, RESP_BAD, "Bogus number set");

  if (strcasecmp (name, "INBOX") == 0)
    {
      struct passwd *pw = getpwuid (getuid());
      mailbox_name = strdup ((pw) ? pw->pw_name : "");
    }
  else
    mailbox_name = util_getfullpath (name, delim);

  /* If the destination mailbox does not exist, a server should return
     an error.  */
  status = mailbox_create_default (&cmbox, mailbox_name);
  if (status == 0)
    {
      /* It SHOULD NOT automatifcllly create the mailbox. */
      status = mailbox_open (cmbox, MU_STREAM_RDWR);
      if (status == 0)
	{
	  size_t i;
	  for (i = 0; i < n; i++)
	    {
	      message_t msg = NULL;
	      mailbox_get_message (mbox, set[i], &msg);
	      mailbox_append_message (cmbox, msg);
	    }
	  mailbox_close (cmbox);
	}
      mailbox_destroy (&cmbox);
    }
  free (set);
  free (mailbox_name);

  if (status == 0)
    return util_finish (command, RESP_OK, "Completed");

  /* Since we do not call util_finish, reset the state ourself.  */
  if (command->failure != STATE_NONE)
    state = command->failure;

  /* Unless it is certain that the destination mailbix can not be created,
     the server MUST send the response code "[TRYCREATE]" as the prefix
     of the text of the tagged NO response.  This gives a hint to the
     client that it can attempt a CREATE command and retry the copy if
     the CREATE is successful.  */
  return util_send ("%s NO [TRYCREATE] failed\r\n", command->tag);
}
