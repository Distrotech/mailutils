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

#include "mail.h"

/*
 * to[p] [msglist]
 */

int
mail_top (int argc, char **argv)
{
  if (argc > 1)
    return util_msglist_command (mail_top, argc, argv);
  else
    {
      message_t msg;
      stream_t stream;
      char buf[BUFSIZ];
      size_t n;
      off_t off;
      int lines = strtol ((util_find_env("toplines"))->value, NULL, 10);
      
      if (lines < 0)
	return 1;

      if (mailbox_get_message (mbox, cursor, &msg) != 0)
	return 1;

      if (util_isdeleted (cursor))
	return 1;

      message_get_stream (msg, &stream);
      for (n = 0, off = 0; lines > 0; lines--, off += n)
	{
	  int status = stream_readline (stream, buf, sizeof (buf), off, &n);
	  if (status != 0 || n == 0)
	    break;
	  printf ("%s", buf);
	}
    }
  return 1;
}


