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

/* Prints out the specified message */

int
pop3_retr (const char *arg)
{
  unsigned int mesg, size, read = 0;
  char buf[BUFFERSIZE];
  message_t msg;
  header_t hdr;
  body_t body;
  stream_t stream;

  if ((strlen (arg) == 0) || (strchr (arg, ' ') != NULL))
    return ERR_BAD_ARGS;

  if (state != TRANSACTION)
    return ERR_WRONG_STATE;

  mesg = atoi (arg);

  if (mailbox_get_message (mbox, mesg, &msg) != 0)
    return ERR_NO_MESG;

  message_get_header (msg, &hdr);
  message_get_body (msg, &body);

  /* Header */
  fprintf (ofile, "+OK\r\n");
  header_get_stream (hdr, &stream);
  header_size (hdr, &size);
  while (read < size)
    {
      stream_read (stream, buf, BUFFERSIZE, 0, NULL);
      fprintf (ofile, "%s", buf);
      read += BUFFERSIZE;
    }


  /* body */
  body_get_stream (body, &stream);
  body_lines (body, &size);
  while (read < size)
    {
      stream_read (stream, buf, BUFFERSIZE, 0, NULL);
      fprintf (ofile, "%s", buf);
      read += BUFFERSIZE;
    }
  fprintf (ofile, ".\r\n");

  return OK;
}

