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

/* Displays the size of message number arg or all messages (if no arg) */

int
pop3d_list (const char *arg)
{
  size_t mesgno;
  size_t size = 0;
  size_t lines = 0;
  message_t msg;
  attribute_t attr;

  if (state != TRANSACTION)
    return ERR_WRONG_STATE;

  if (strchr (arg, ' ') != NULL)
    return ERR_BAD_ARGS;

  if (strlen (arg) == 0)
    {
      size_t total = 0;
      fprintf (ofile, "+OK\r\n");
      mailbox_messages_count (mbox, &total);
      for (mesgno = 1; mesgno <= total; mesgno++)
	{
	  mailbox_get_message (mbox, mesgno, &msg);
	  message_get_attribute (msg, &attr);
	  if (!attribute_is_deleted (attr))
	    {
	      message_size (msg, &size);
	      message_lines (msg, &lines);
	      fprintf (ofile, "%d %d\r\n", mesgno, size + lines);
	    }
	}
      fprintf (ofile, ".\r\n");
    }
  else
    {
      mesgno = strtoul (arg, NULL, 10);
      if (mailbox_get_message (mbox, mesgno, &msg) != 0)
	return ERR_NO_MESG;
      message_get_attribute (msg, &attr);
      if (attribute_is_deleted (attr))
	return ERR_MESG_DELE;
      message_size (msg, &size);
      message_lines (msg, &lines);
      fprintf (ofile, "+OK %d %d\r\n", mesgno, size + lines);
    }

  return OK;
}
