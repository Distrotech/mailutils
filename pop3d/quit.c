/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999 Free Software Foundation, Inc.

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

int
pop3_quit (const char *arg)
{
  if (strlen (arg) != 0)
    return ERR_BAD_ARGS;

  if (state == TRANSACTION)
    {
      mailbox_expunge (mbox);
      mailbox_close (mbox);
      mailbox_destroy (&mbox);
      syslog (LOG_INFO, "Session ended for user: %s", username);
    }
  else
    syslog (LOG_INFO, "Session ended for no user");

  state = UPDATE;
  free (username);
  free (md5shared);

  fprintf (ofile, "+OK\r\n");
  return OK;
}




