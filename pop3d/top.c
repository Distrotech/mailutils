/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

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

/* Prints the header of a message plus a specified number of lines.  */

int
pop3_top (const char *arg)
{
  size_t mesgno;
  int lines;
  message_t msg;
  attribute_t attr;
  header_t hdr;
  body_t body;
  stream_t stream;
  char *mesgc, *linesc;
  char buf[BUFFERSIZE];
  size_t n;
  off_t off;

  if (strlen (arg) == 0)
    return ERR_BAD_ARGS;

  if (state != TRANSACTION)
    return ERR_WRONG_STATE;

  mesgc = pop3_cmd (arg);
  linesc = pop3_args (arg);
  mesgno = strtoul (mesgc, NULL, 10);
  lines = strlen (linesc) > 0 ? strtoul (linesc, NULL, 10) : -1;
  free (mesgc);
  free (linesc);

  if (lines < 0)
    return ERR_BAD_ARGS;

  if (mailbox_get_message (mbox, mesgno, &msg) != 0)
    return ERR_NO_MESG;

  message_get_attribute (msg, &attr);
  if (attribute_is_deleted (attr))
    return ERR_MESG_DELE;

  fprintf (ofile, "+OK\r\n");

  /* Header.  */
  message_get_header (msg, &hdr);
  header_get_stream (hdr, &stream);
  off = n = 0;
  while (stream_readline (stream, buf, sizeof (buf) - 1, off, &n) == 0)
    {
      if (n == 0)
	break;
      /* Nuke the trainline newline.  */
      if (buf[n - 1] == '\n')
	buf [n - 1] = '\0';
      off += n;
      fprintf (ofile, "%s\r\n", buf);
    }

  /* Lines of body.  */
  if (lines)
    {
      message_get_body (msg, &body);
      body_get_stream (body, &stream);
      for (n = off = 0; lines > 0; lines--, off += n)
	{
	  int status = stream_readline (stream, buf, sizeof (buf), off, &n);
	  if (status != 0 || n == 0)
	    break;
	  /* Nuke the trainline newline.  */
	  if (buf[n - 1] == '\n')
	    buf[n - 1] = '\0';
	  if (buf[0] == '.')
	    fprintf (ofile, ".%s\r\n", buf);
	  else
	    fprintf (ofile, "%s\r\n", buf);
	}
    }
  fprintf (ofile, ".\r\n");

  return OK;
}
