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
 * p[rint] [msglist]
 * t[ype] [msglist]
 */

int
mail_print (int argc, char **argv)
{

  if (argc > 1)
    return util_msglist_command (mail_print, argc, argv);
  else
    {
      message_t mesg;
      header_t hdr;
      body_t body;
      stream_t stream;
      char buffer[BUFSIZ];
      off_t off = 0;
      size_t n = 0;

      if (mailbox_get_message (mbox, cursor, &mesg) != 0)
	{
	  printf ("Could not read message %d\n", cursor);
	  return 1;
	}

      message_get_header (mesg, &hdr);
      if (header_get_value (hdr, MU_HEADER_FROM, buffer, sizeof (buffer),
			    NULL) == 0)
	{
	  printf ("From: %s\n", buffer);
	  /* free (buf); */
	}
      if (header_get_value (hdr, MU_HEADER_SUBJECT, buffer, sizeof (buffer),
			    NULL) == 0)
	{
	  printf ("Subject: %s\n", buffer);
	  /* free (buf); */
	}

      message_get_body (mesg, &body);
      body_get_stream (body, &stream);
      while (stream_read (stream, buffer, sizeof (buffer) - 1, off, &n) == 0
             && n != 0)
        {
          buffer[n] = '\0';
          printf ("%s", buffer);
          off += n;
        }
      return 0;
    }
  return 1;
}
