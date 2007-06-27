/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001, 2005, 2007 Free Software Foundation, Inc.

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

/*
 * copy messages in argv[2] to mailbox in argv[3]
 */

int
imap4d_copy (struct imap4d_command *command, char *arg)
{
  int rc;
  char buffer[64];

  rc = imap4d_copy0 (arg, 0, buffer, sizeof buffer);
  if (rc == RESP_NONE)
    {
      /* Reset the state ourself.  */
      int new_state = (rc == RESP_OK) ? command->success : command->failure;
      if (new_state != STATE_NONE)
	state = new_state;
      return util_send ("%s %s\r\n", command->tag, buffer);
    }
  return util_finish (command, rc, "%s", buffer);
}

int
imap4d_copy0 (char *arg, int isuid, char *resp, size_t resplen)
{
  int status;
  char *msgset;
  char *name;
  char *mailbox_name;
  const char *delim = "/";
  char *sp = NULL;
  size_t *set = NULL;
  int n = 0;
  mu_mailbox_t cmbox = NULL;

  msgset = util_getword (arg, &sp);
  name = util_getword (NULL, &sp);

  util_unquote (&name);
  if (!msgset || !name || *name == '\0')
    {
      snprintf (resp, resplen, "Too few args");
      return RESP_BAD;
    }

  /* Get the message numbers in set[].  */
  status = util_msgset (msgset, &set, &n, isuid);
  if (status != 0)
    {
      snprintf (resp, resplen, "Bogus number set");
      return RESP_BAD;
    }

  mailbox_name = namespace_getfullpath (name, delim);

  if (!mailbox_name)
    {
      snprintf (resp, resplen, "NO Create failed.");
      return RESP_NO;
    }

  /* If the destination mailbox does not exist, a server should return
     an error.  */
  status = mu_mailbox_create_default (&cmbox, mailbox_name);
  if (status == 0)
    {
      /* It SHOULD NOT automatifcllly create the mailbox. */
      status = mu_mailbox_open (cmbox, MU_STREAM_RDWR);
      if (status == 0)
	{
	  size_t i;
	  for (i = 0; i < n; i++)
	    {
	      mu_message_t msg = NULL;
	      size_t msgno = (isuid) ? uid_to_msgno (set[i]) : set[i];
	      if (msgno && mu_mailbox_get_message (mbox, msgno, &msg) == 0)
		mu_mailbox_append_message (cmbox, msg);
	    }
	  mu_mailbox_close (cmbox);
	}
      mu_mailbox_destroy (&cmbox);
    }
  free (set);
  free (mailbox_name);

  if (status == 0)
    {
      snprintf (resp, resplen, "Completed");
      return RESP_OK;
    }

  /* Unless it is certain that the destination mailbox can not be created,
     the server MUST send the response code "[TRYCREATE]" as the prefix
     of the text of the tagged NO response.  This gives a hint to the
     client that it can attempt a CREATE command and retry the copy if
     the CREATE is successful.  */
  snprintf (resp, resplen, "NO [TRYCREATE] failed");
  return RESP_NONE;
}
