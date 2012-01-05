/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2002, 2005, 2007, 2009-2012 Free Software
   Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>. */

#include "mail.h"

/*
 * p[rint] [msglist]
 * t[ype] [msglist]
 * P[rint] [msglist]
 * T[ype] [msglist]
 */

static int
mail_print_msg (msgset_t *mspec, mu_message_t mesg, void *data)
{
  mu_header_t hdr;
  mu_body_t body;
  mu_stream_t stream;
  size_t lines = 0;
  mu_stream_t out;
  int pagelines = util_get_crt ();
  int status;
  
  mu_message_lines (mesg, &lines);
  if (mailvar_get (NULL, "showenvelope", mailvar_type_boolean, 0) == 0)
    lines++;
  
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
	      mu_message_size (mesg, &size);
	      lines =  size / col;
	    }
	}
    }

  out = open_pager (lines);

  if (mailvar_get (NULL, "showenvelope", mailvar_type_boolean, 0) == 0)
    print_envelope (mspec, mesg, "From");
  
  if (*(int *) data) /* print was called with a lowercase 'p' */
    {
      size_t i, num = 0;
      const char *sptr;
      char *tmp;
      
      mu_message_get_header (mesg, &hdr);
      mu_header_get_field_count (hdr, &num);

      for (i = 1; i <= num; i++)
	{
	  if (mu_header_sget_field_name (hdr, i, &sptr))
	    continue;
	  if (mail_header_is_visible (sptr))
	    {
	      mu_stream_printf (out, "%s: ", sptr);
	      mu_header_aget_field_value (hdr, i, &tmp);
	      if (mail_header_is_unfoldable (sptr))
		mu_string_unfold (tmp, NULL);
	      util_rfc2047_decode (&tmp);
	      mu_stream_printf (out, "%s\n", tmp);
	      free (tmp);
	    }
	}
      mu_stream_printf (out, "\n");
      mu_message_get_body (mesg, &body);
      status = mu_body_get_streamref (body, &stream);
    }
  else
    status = mu_message_get_streamref (mesg, &stream);

  if (status)
    {
      mu_error (_("get_stream error: %s"), mu_strerror (status));
      mu_stream_unref (out);
      return 0;
    }

  mu_stream_copy (out, stream, 0, NULL);
  /* FIXME:
      if (ml_got_interrupt())
	{
	  mu_error (_("\nInterrupt"));
	  break;
	}
  */
  mu_stream_destroy (&stream);
  mu_stream_destroy (&out);
  
  util_mark_read (mesg);

  set_cursor (mspec->msg_part[0]);
  
  return 0;
}

int
mail_print (int argc, char **argv)
{
  int lower = mu_islower (argv[0][0]);
  int rc = util_foreach_msg (argc, argv, MSG_NODELETED|MSG_SILENT,
			     mail_print_msg, &lower);
  return rc;
}

