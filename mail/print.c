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
 * p[rint] [msglist]
 * t[ype] [msglist]
 */

int
mail_print (int argc, char **argv)
{
  message_t mesg;
  header_t hdr;
  body_t body;
  stream_t stream;

  if (argc > 1)
    return util_msglist_command (mail_print, argc, argv);
  else if (mailbox_get_message (mbox, cursor, &mesg) != 0)
    printf ("Couldn't read message %d\n", cursor);
  else if (message_get_header (mesg, &hdr) != 0)
    printf ("Couldn't read message header on message %d\n", cursor);
  else if (message_get_body (mesg, &body) != 0)
    printf ("Couldn't read message body from message %d\n", cursor);
  else if (body_get_stream (body, &stream) != 0)
    printf ("Couldn't get stream for message %d\n", cursor);
  else
    {
      char *buf = NULL;
      size_t len = 0;
      buf = malloc (80 * sizeof (char));
      if (header_get_value (hdr, MU_HEADER_FROM, buf, 80, NULL) == 0)
	{
	  printf ("From: %s\n", buf);
	  /* free (buf); */
	}
      if (header_get_value (hdr, MU_HEADER_SUBJECT, buf, 80,NULL) == 0)
	{
	  printf ("Subject: %s\n", buf);
	  /* free (buf); */
	}
      free (buf);
      body_size (body, &len);
      buf = malloc ((len+1) * sizeof(char));
      memset (buf, '\0', len+1);
      stream_read (stream, buf, len, 0, NULL);
      printf ("\n%s\n", buf);
      free (buf);
    }

  return 1;
}
