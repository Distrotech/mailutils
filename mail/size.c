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
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA  */

#include "mail.h"

/*
 * si[ze] [msglist]
 */

static int
size0 (msgset_t *mspec, message_t msg, void *data)
{
  size_t size = 0, lines = 0;

  message_size (msg, &size);
  message_lines (msg, &lines);
  
  fprintf (ofile, "%c%2d %3d/%-5d\n",
	   mspec->msg_part[0] == cursor ? '>' : ' ',
	   mspec->msg_part[0], lines, size);
  return 0;
}

int
mail_size (int argc, char **argv)
{
  return util_foreach_msg (argc, argv, MSG_ALL, size0, NULL);
}


