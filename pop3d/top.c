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

/* Prints the header of a message plus a specified number of lines */

int
pop3_top (const char *arg)
{
  int mesg, lines, size;
  message_t msg;
  header_t hdr;
  body_t body;
  stream_t stream;
  char *mesgc, *linesc;
  char *buf;

  if (strlen (arg) == 0)
    return ERR_BAD_ARGS;

  if (state != TRANSACTION)
    return ERR_WRONG_STATE;

  mesgc = pop3_cmd (arg);
  linesc = pop3_args (arg);
  mesg = atoi (mesgc);
  lines = strlen (linesc) > 0 ? atoi (linesc) : -1;
  free (mesgc);
  free (linesc);

  if (lines < 0)
    return ERR_BAD_ARGS;

  if (mailbox_get_message (mbox, mesg, &msg) != 0)
    return ERR_NO_MESG;

  message_get_header (msg, &hdr);
  message_get_body (msg, &body);

  /* Header */
  header_get_stream (hdr, &stream);
  header_size (hdr, &size);
  buf = malloc (size);
  stream_read (stream, buf, size, 0, NULL);
  fprintf (ofile, "+OK\r\n%s", buf);
  free(buf);

  /* lines lines of body */
  body_get_stream (body, &stream);
  body_lines (body, &size);
  buf = malloc (size);
  stream_read (stream, buf, size, 0, NULL);	/* FIXME: read lines lines */
  fprintf (ofile, "%s.\r\n", buf);
  free (buf);

  return OK;
}

