/* GNU POP3 - a small, fast, and efficient POP3 daemon
   Copyright (C) 1999 Jakob 'sparky' Kaivo <jkaivo@nodomainname.net>

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

/* Prints out the specified message */

int
pop3_retr (const char *arg)
{
  int mesg;
  char *buf;

  if ((strlen (arg) == 0) || (strchr (arg, ' ') != NULL))
    return ERR_BAD_ARGS;

  if (state != TRANSACTION)
    return ERR_WRONG_STATE;

  mesg = atoi (arg) - 1;

  if (mesg > mbox->messages || mbox_is_deleted(mbox, mesg))
    return ERR_NO_MESG;

  fprintf (ofile, "+OK\r\n");
  buf = mbox_get_header (mbox, mesg);
  fprintf (ofile, "%s", buf);
  free (buf);
  buf = mbox_get_body (mbox, mesg);
  fprintf (ofile, "%s", buf);
  free (buf);
  fprintf (ofile, ".\r\n");
  return OK;
}
