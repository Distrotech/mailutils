/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002 Free Software Foundation, Inc.

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

#include "pop3d.h"

/* Enters the UPDATE phase and deletes marked messages */
/* Note:
   Whether the removal was successful or not, the server
   then releases any exclusive-access lock on the maildrop
   and closes the TCP connection.  */

static void pop3d_fix_mark ();

int
pop3d_quit (const char *arg)
{
  int err = OK;
  if (strlen (arg) != 0)
    return ERR_BAD_ARGS;

  if (state == TRANSACTION)
    {
      pop3d_unlock ();
      pop3d_fix_mark ();

      if (mailbox_flush (mbox, 1) != 0)
	err = ERR_FILE;
      if (mailbox_close (mbox) != 0) 
	err = ERR_FILE;
      mailbox_destroy (&mbox);
      syslog (LOG_INFO, _("Session ended for user: %s"), username);
    }
  else
    syslog (LOG_INFO, _("Session ended for no user"));

  state = UPDATE;
  free (username);
  free (md5shared);

  if (err == OK)
    pop3d_outf ("+OK\r\n");
  return err;
}

static void
pop3d_fix_mark ()
{
  size_t i;
  size_t total = 0;

  mailbox_messages_count (mbox, &total);

  for (i = 1; i <= total; i++)
    {
       message_t msg = NULL;
       attribute_t attr = NULL;
       mailbox_get_message (mbox, i, &msg);
       message_get_attribute (msg, &attr);
       if (attribute_is_userflag (attr, POP3_ATTRIBUTE_DELE))
          attribute_set_deleted (attr);
    }
}
