/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2001 Free Software Foundation, Inc.

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
 * dec[ode] ???
 * FIXME: the sytnax of this needs to be discussed on bug-mailutils
 */

int
mail_decode (int argc, char **argv)
{
#if 0
  fprintf (stderr, "Sorry, the decode function hasn't been implemented yet\n");
  return 1;
#else
  message_t msg = NULL;
  int partno;
  int *list = NULL;
  int num = util_expand_msglist (argc, argv, &list);

  partno = (num >= 2) ? list[1] : 1;

  if (mailbox_get_message (mbox, list[0], &msg) == 0)
    {
      message_t part = NULL;
      if (message_get_part (msg, partno, &part) == 0)
	{
	  body_t body = NULL;
	  header_t hdr = NULL;
	  stream_t stream = NULL;
	  stream_t b_stream = NULL;
	  size_t len = 0;
	  char *encoding = NULL;

	  message_get_body (part, &body);
	  body_get_stream (body, &b_stream);
	  message_get_header (part, &hdr);
	  if (header_aget_value (hdr, MU_HEADER_CONTENT_TRANSFER_ENCODING,
				 &encoding) == 0)
	    {
	      size_t lines = 0;
	      char buffer[512];
	      off_t off = 0;
	      size_t n = 0;
	      FILE *out = ofile;
	      stream_t d_stream = NULL;

	      message_lines (part, &lines);
	      if ((util_find_env("crt"))->set && lines > util_getlines ())
		out = popen (getenv("PAGER"), "w");

	      /* Do I understand this encoding?  */
	      if (*encoding
		  && filter_create(&d_stream, b_stream, encoding,
				   MU_FILTER_DECODE, MU_STREAM_READ) == 0)
		stream = d_stream;
	      else
		stream = b_stream;

	      while (stream_read (stream, buffer, sizeof buffer,
				  off, &n ) == 0 && n)
		{
		  if (ml_got_interrupt())
		    {
		      util_error("\nInterrupt");
		      break;
		    }
		  buffer[n] = '\0';
		  fprintf (out, "%s", buffer);
		  off += n;
		}
	      if (d_stream)
		stream_destroy (&d_stream, NULL);
	      if (out != ofile)
		pclose (out);
	      free (encoding);
	    }
	}
    }
  free (list);
#endif
}
