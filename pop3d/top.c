/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

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

#include "pop3d.h"

/* Prints the header of a message plus a specified number of lines.  */

int
pop3d_top (const char *arg)
{
  size_t mesgno;
  int lines;
  message_t msg;
  attribute_t attr;
  header_t hdr;
  body_t body;
  stream_t stream;
  char *mesgc, *linesc;
  char *buf;
  size_t buflen = BUFFERSIZE;
  size_t n;
  off_t off;

  if (strlen (arg) == 0)
    return ERR_BAD_ARGS;

  if (state != TRANSACTION)
    return ERR_WRONG_STATE;

  mesgc = pop3d_cmd (arg);
  linesc = pop3d_args (arg);
  mesgno = strtoul (mesgc, NULL, 10);
  lines = strlen (linesc) > 0 ? strtol (linesc, NULL, 10) : -1;
  free (mesgc);
  free (linesc);

  if (lines < 0)
    return ERR_BAD_ARGS;

  if (mailbox_get_message (mbox, mesgno, &msg) != 0)
    return ERR_NO_MESG;

  message_get_attribute (msg, &attr);
  if (attribute_is_deleted (attr))
    return ERR_MESG_DELE;

  pop3d_outf ("+OK\r\n");

  /* Header.  */
  message_get_header (msg, &hdr);
  header_get_stream (hdr, &stream);
  buf = malloc (buflen * sizeof (*buf));
  if (buf == NULL)
    pop3d_abquit (ERR_NO_MEM);
  off = n = 0;
  while (stream_readline (stream, buf, buflen, off, &n) == 0
	 && n > 0)
    {
      /* Nuke the trainline newline.  */
      if (buf[n - 1] == '\n')
	{
	  buf[n - 1] = '\0';
	  pop3d_outf ("%s\r\n", buf);
	}
      else
	pop3d_outf ("%s", buf);
      off += n;
    }

  /* Lines of body.  */
  if (lines)
    {
      message_get_body (msg, &body);
      body_get_stream (body, &stream);
      n = off = 0;
      while (stream_readline (stream, buf, buflen, off, &n) == 0
	     && n > 0 && lines > 0)
	{
	  /* Nuke the trailing newline.  */
	  if (buf[n - 1] == '\n')
	    buf[n - 1] = '\0';
	  else /* make room for the line.  */
	    {
	      buflen *= 2;
	      buf = realloc (buf, buflen * sizeof (*buf));
	      if (buf == NULL)
		pop3d_abquit (ERR_NO_MEM);
	      continue;
	    }
	  if (buf[0] == '.')
	    pop3d_outf (".%s\r\n", buf);
	  else
	    pop3d_outf ("%s\r\n", buf);
	  lines--;
	  off += n;
	}
    }

  free (buf);
  pop3d_outf (".\r\n");

  return OK;
}
