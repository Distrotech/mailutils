/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

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

#include "pop3d.h"

/* Enters the UPDATE phase and deletes marked messages */
/* Note:
   Whether the removal was successful or not, the server
   then releases any exclusive-access lock on the maildrop
   and closes the TCP connection.  */

int
pop3d_quit (const char *arg)
{
  int err = OK;
  if (strlen (arg) != 0)
    return ERR_BAD_ARGS;

  if (state == TRANSACTION)
    {
      pop3d_unlock ();
      if (mailbox_expunge (mbox) != 0)
	err = ERR_FILE;
      if (mailbox_close (mbox) != 0)
	err = ERR_FILE;
      mailbox_destroy (&mbox);
      syslog (LOG_INFO, "Session ended for user: %s", username);
    }
  else
    syslog (LOG_INFO, "Session ended for no user");

  state = UPDATE;
  free (username);
  free (md5shared);

  if (err == OK)
    fprintf (ofile, "+OK\r\n");
  return err;
}
