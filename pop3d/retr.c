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

#include "pop3d.h"

/* Prints out the specified message */

int
pop3_retr (const char *arg)
{
  size_t mesgno, n;
  char buf[BUFFERSIZE];
  message_t msg;
  attribute_t attr;
  stream_t stream;
  off_t off;

  if ((strlen (arg) == 0) || (strchr (arg, ' ') != NULL))
    return ERR_BAD_ARGS;

  if (state != TRANSACTION)
    return ERR_WRONG_STATE;

  mesgno = atoi (arg);

  if (mailbox_get_message (mbox, mesgno, &msg) != 0)
    return ERR_NO_MESG;

  message_get_attribute (msg, &attr);
  if (attribute_is_deleted (attr))
    return ERR_MESG_DELE;

  message_get_stream (msg, &stream);
  fprintf (ofile, "+OK\r\n");
  off = n = 0;
  while (stream_readline (stream, buf, sizeof (buf) - 1, off, &n) == 0
	 && n > 0)
    {
      /* Nuke the trainline newline.  */
      if (buf[n - 1] == '\n')
	buf[n - 1] = '\0';
      if (buf[0] == '.')
	fprintf (ofile, ".%s\r\n", buf);
      else
	fprintf (ofile, "%s\r\n", buf);
      off += n;
    }

  fprintf (ofile, ".\r\n");

  return OK;
}
