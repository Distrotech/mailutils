/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000 Free Software Foundation, Inc.

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
 * FIXME:  Need to check for toplines variable value, how ?
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
      int toplines = 5; /* FIXME: Use variable TOPLINES.  */
      off_t off = 0;
      size_t n = 0;
      char buf[BUFSIZ];
      int status;

      if (mailbox_get_message (mbox, cursor, &msg) != 0)
        {
          fprintf (stderr, "Could not read message %d\n", cursor);
          return 1;
        }

      message_get_stream (msg, &stream);
      while (toplines--)
        {
          status = stream_readline (stream, buf, sizeof (buf), off, &n);
          if (status != 0 || n == 0)
            break;
	  printf ("%s", buf);
	  off += n;
	}
      return 0;
    }
  return 1;
}
