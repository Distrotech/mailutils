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

/* Displays the size of message number arg or all messages (if no arg) */

int
pop3_list (const char *arg)
{
  int mesg = 0;

  if (state != TRANSACTION)
    return ERR_WRONG_STATE;

  if (strchr (arg, ' ') != NULL)
    return ERR_BAD_ARGS;

  if (strlen (arg) == 0)
    {
      fprintf (ofile, "+OK\r\n");
      for (mesg = 0; mesg < mbox->messages ; mesg++)
	if (!mbox_is_deleted(mbox, mesg))
	  fprintf (ofile, "%d %d\r\n", mesg + 1, mbox->sizes[mesg]);
      fprintf (ofile, ".\r\n");
    }
  else
    {
      mesg = atoi (arg) - 1;
      if (mesg > mbox->messages || mbox_is_deleted(mbox, mesg))
	return ERR_NO_MESG;
      fprintf (ofile, "+OK %d %d\r\n", mesg + 1, mbox->sizes[mesg]);
    }

  return OK;
}
