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

#include "mail.h"

/*
 * si[ze] [msglist]
 */

int
mail_size (int argc, char **argv)
{
  if (argc > 1)
    util_msglist_command (mail_size, argc, argv);
  else
    {
      unsigned int s = 0;
      message_t msg;
      if (mailbox_get_message (mbox, cursor, &msg) != 0)
	{
	  fprintf (stderr, "Could not read message %d\n", cursor);
	  return 1;
	}
      message_size (msg, &s);
      fprintf (ofile, "%c%2d %d\n", cursor == realcursor ? '>' : ' ',
	       cursor, s);
      return 0;
    }
  return 1;
}
