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

#include "mail.h"

/*
 * p[rint] [msglist]
 * t[ype] [msglist]
 * P[rint] [msglist]
 * T[ype] [msglist]
 */

int
mail_print (int argc, char **argv)
{
  if (argc > 1)
    return util_msglist_command (mail_print, argc, argv, 1);
  else
    {
      message_t mesg;
      header_t hdr;
      body_t body;
      stream_t stream;
      char buffer[512];
      off_t off = 0;
      size_t n = 0, lines = 0;
      FILE *out = ofile;
      attribute_t attr;
      
      if (mailbox_get_message (mbox, cursor, &mesg) != 0)
	return 1;

      if (util_isdeleted (cursor))
        return 1;

      message_lines (mesg, &lines);

      if ((util_find_env("crt"))->set && lines > util_getlines ())
	    out = popen (getenv("PAGER"), "w");

      if (islower (argv[0][0]))
	{
	  size_t i, num = 0;
	  char buffer[512];
	  
	  message_get_header (mesg, &hdr);
	  header_get_field_count (hdr, &num);

	  for (i = 1; i <= num; i++)
	    {
	      header_get_field_name (hdr, i, buffer, sizeof(buffer), NULL);
	      if (mail_header_is_visible (buffer))
		{
		  fprintf (out, "%s: ", buffer);
		  header_get_field_value (hdr, i, buffer, sizeof(buffer),
					  NULL);
		  fprintf (out, "%s\n", buffer);
		}
	    }
	  fprintf (out, "\n");
	  message_get_body (mesg, &body);
	  body_get_stream (body, &stream);
	}
      else
	message_get_stream (mesg, &stream);

      while (stream_read (stream, buffer, sizeof (buffer) - 1, off, &n) == 0
             && n != 0)
        {
          buffer[n] = '\0';
	  fprintf (out, "%s", buffer);
          off += n;
        }
      if (out != ofile)
	pclose (out);

      message_get_attribute (mesg, &attr);
      attribute_set_read (attr);
      
      return 0;
    }
  return 1;
}


