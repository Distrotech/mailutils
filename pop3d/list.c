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
  unsigned int mesg = 0, size = 0;
  message_t msg;

  if (state != TRANSACTION)
    return ERR_WRONG_STATE;

  if (strchr (arg, ' ') != NULL)
    return ERR_BAD_ARGS;

  /* FIXME: how to find if mailbox is deleted, how to get size */

  if (strlen (arg) == 0)
    {
      unsigned int total;
      mailbox_messages_count (mbox, &total);
      fprintf (ofile, "+OK\r\n");
      for (mesg = 1; mesg <= total; mesg++)
	{
	  mailbox_get_message (mbox, mesg, &msg);
	  if ( /* deleted == 0 */ 1)
	    {
	      message_size (msg, &size);
	      fprintf (ofile, "%d %d\r\n", mesg, size);
	    }
	}
      fprintf (ofile, ".\r\n");
    }
  else
    {
      mesg = atoi (arg);
      if (mailbox_get_message (mbox, mesg, &msg) != 0)
	return ERR_NO_MESG;
      message_size (msg, &size);
      fprintf (ofile, "+OK %d %d\r\n", mesg, size);
    }

  return OK;
}
