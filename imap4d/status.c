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
 *
 */

static int status_messages    __P ((mailbox_t));
static int status_recent      __P ((mailbox_t));
static int status_uidnext     __P ((mailbox_t));
static int status_uidvalidity __P ((mailbox_t));
static int status_unseen      __P ((mailbox_t));

int
imap4d_status (struct imap4d_command *command, char *arg)
{
  char *sp = NULL;
  char *name;
  char *mailbox_name;
  const char *delim = "/";
  mailbox_t smbox = NULL;
  int status;

  if (! (command->states & state))
    return util_finish (command, RESP_BAD, "Wrong state");

  name = util_getword (arg, &sp);
  util_unquote (&name);
  if (!name || *name == '\0' || !sp || *sp == '\0')
    return util_finish (command, RESP_BAD, "Too few args");

  if (strcasecmp (name, "INBOX") == 0)
    {
      struct passwd *pw = getpwuid (getuid());
      mailbox_name = strdup ((pw) ? pw->pw_name : "");
    }
  else
    mailbox_name = util_getfullpath (name, delim);

  status = mailbox_create_default (&smbox, mailbox_name);
  if (status == 0)
    {
      status = mailbox_open (smbox, MU_STREAM_READ);
      if (status == 0)
	{
	  char item[32];
	  util_send ("* STATUS %s (", name);
	  item[0] = '\0';
	  /* Get the status item names.  */
	  while (*sp && *sp != ')')
	    {
	      int err = 1;
	      util_token (item, sizeof (item), &sp);
	      if (strcasecmp (item, "MESSAGES") == 0)
		err = status_messages (smbox);
	      else if (strcasecmp (item, "RECENT") == 0)
		err = status_recent (smbox);
	      else if (strcasecmp (item, "UIDNEXT") == 0)
		err = status_uidnext (smbox);
	      else if (strcasecmp (item, "UIDVALIDITY") == 0)
		err = status_uidvalidity (smbox);
	      else if (strcasecmp (item, "UNSEEN") == 0)
		err = status_unseen (smbox);
	      if (!err)
		util_send (" ");
	    }
	  util_send (")\r\n");
	  mailbox_close (smbox);
	}
      mailbox_destroy (&smbox);
    }
  free (mailbox_name);

  if (status == 0)
    return util_finish (command, RESP_OK, "Completed");
  return util_finish (command, RESP_NO, "Error opening mailbox");
}

static int
status_messages (mailbox_t smbox)
{
  size_t total = 0;
  mailbox_messages_count (smbox, &total);
  util_send ("MESSAGES %u", total);
  return 0;
}

static int
status_recent (mailbox_t smbox)
{
  size_t recent = 0;
  mailbox_messages_recent (smbox, &recent);
  util_send ("RECENT %u", recent);
  return 0;
}

static int
status_uidnext (mailbox_t smbox)
{
  size_t uidnext = 1;
  mailbox_uidnext (smbox, &uidnext);
  util_send ("UIDNEXT %u", uidnext);
  return 0;
}

static int
status_uidvalidity (mailbox_t smbox)
{
  unsigned long uidvalidity = 0;
  mailbox_uidvalidity (smbox, &uidvalidity);
  util_send ("UIDVALIDITY %u", uidvalidity);
  return 0;
}

/* Note that unlike the unseen response code, which indicates the message
   number of the first unseen message, the unseeen item in the response the
   status command indicates the quantity of unseen messages.  */
static int
status_unseen (mailbox_t smbox)
{
  size_t total = 0;
  size_t unseen = 0;
  size_t i;
  mailbox_messages_count (smbox, &total);
  for (i = 1; i <= total; i++)
    {
      message_t msg = NULL;
      attribute_t attr = NULL;
      mailbox_get_message (smbox, i, &msg);
      message_get_attribute (msg, &attr);
      if (!attribute_is_seen (attr) && !attribute_is_read (attr))
	unseen++;
    }
  util_send ("UNSEEN %d", unseen);
  return 0;

}
