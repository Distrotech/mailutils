/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002, 2005 Free Software Foundation, Inc.

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
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA  */

#include "mail.h"

/*
 * p[rint] [msglist]
 * t[ype] [msglist]
 * P[rint] [msglist]
 * T[ype] [msglist]
 */

static int
mail_print_msg (msgset_t *mspec, message_t mesg, void *data)
{
  header_t hdr;
  body_t body;
  stream_t stream;
  char buffer[512];
  off_t off = 0;
  size_t n = 0, lines = 0;
  FILE *out = ofile;
  attribute_t attr;
  int pagelines = util_get_crt ();
  
  message_lines (mesg, &lines);

  /* If it is POP or IMAP the lines number is not known, so try
     to be smart about it.  */
  if (lines == 0)
    {
      if (pagelines)
	{
	  size_t col = (size_t)util_getcols ();
	  if (col)
	    {
	      size_t size = 0;
	      message_size (mesg, &size);
	      lines =  size / col;
	    }
	}
    }

  if (pagelines && lines > pagelines)
    out = popen (getenv ("PAGER"), "w");

  if (*(int *) data) /* print was called with a lowercase 'p' */
    {
      size_t i, num = 0;
      char buf[512];
      char *tmp;
      
      message_get_header (mesg, &hdr);
      header_get_field_count (hdr, &num);

      for (i = 1; i <= num; i++)
	{
	  header_get_field_name (hdr, i, buf, sizeof buf, NULL);
	  if (mail_header_is_visible (buf))
	    {
	      fprintf (out, "%s: ", buf);
	      header_aget_field_value (hdr, i, &tmp);
	      if (mail_header_is_unfoldable (buf))
		mu_string_unfold (tmp, NULL);
	      util_rfc2047_decode (&tmp);
	      fprintf (out, "%s\n", tmp);
	      free (tmp);
	    }
	}
      fprintf (out, "\n");
      message_get_body (mesg, &body);
      body_get_stream (body, &stream);
    }
  else
    message_get_stream (mesg, &stream);
  
  while (stream_read (stream, buffer, sizeof buffer - 1, off, &n) == 0
	 && n != 0)
    {
      if (ml_got_interrupt())
	{
	  util_error (_("\nInterrupt"));
	  break;
	}
      buffer[n] = '\0';
      fprintf (out, "%s", buffer);
      off += n;
    }
  if (out != ofile)
    pclose (out);
  
  message_get_attribute (mesg, &attr);
  attribute_set_read (attr);
  attribute_set_userflag (attr, MAIL_ATTRIBUTE_SHOWN);

  cursor = mspec->msg_part[0];
  
  return 0;
}

int
mail_print (int argc, char **argv)
{
  int lower = islower (argv[0][0]);
  int rc = util_foreach_msg (argc, argv, MSG_NODELETED|MSG_SILENT,
			     mail_print_msg, &lower);
  return rc;
}

