/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001 Free Software Foundation, Inc.

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
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#include "mail.h"

/*
 * si[ze] [msglist]
 */

int
mail_size (int argc, char **argv)
{
  if (argc > 1)
    util_msglist_command (mail_size, argc, argv, 0);
  else
    {
      size_t size = 0, lines = 0;
      message_t msg;

      if (util_get_message (mbox, cursor, &msg, MSG_ALL))
	return 1;

      message_size (msg, &size);
      message_lines (msg, &lines);

      fprintf (ofile, "%c%2d %3d/%-5d\n", cursor == realcursor ? '>' : ' ',
	       cursor, lines, size);
      return 0;
    }
  return 1;
}
