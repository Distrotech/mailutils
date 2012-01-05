/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001-2003, 2005, 2007, 2009-2012 Free Software
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
 * w[rite] [file] -- GNU extension
 * w[rite] [msglist] file
 * W[rite] [msglist] -- GNU extension
 */

int
mail_write (int argc, char **argv)
{
  int rc;
  mu_stream_t output;
  char *filename = NULL;
  msgset_t *msglist = NULL, *mp;
  int sender = 0;
  size_t total_size = 0, total_lines = 0, size;

  if (mu_isupper (argv[0][0]))
    sender = 1;
  else if (argc >= 2)
    filename = util_outfolder_name (argv[--argc]);
  else
    {
      size_t n = get_cursor ();
      char *p = NULL;
      if (n == 0)
        {
          mu_error (_("No applicable message"));
          return 1;
        }
      mu_asprintf (&p, "%lu", (unsigned long) n);
      filename = util_outfolder_name (p);
      free (p);
    }
		
  if (msgset_parse (argc, argv, MSG_NODELETED|MSG_SILENT, &msglist))
    {
      if (filename)
	free (filename);
      return 1;
    }

  if (sender)
    {
      filename = util_outfolder_name (util_get_sender(msglist->msg_part[0], 1));
      if (!filename)
	{
	  msgset_free (msglist);
	  return 1;
	}
    }

  rc = mu_file_stream_create (&output, filename,
			      MU_STREAM_APPEND|MU_STREAM_CREAT);
  if (rc)
    {
      mu_error (_("can't open %s: %s"), filename, mu_strerror (rc));
      free (filename);
      msgset_free (msglist);
      return 1;
    }

  for (mp = msglist; mp; mp = mp->next)
    {
      mu_message_t msg;
      mu_body_t body;
      mu_stream_t stream;
      mu_attribute_t attr;

      if (util_get_message (mbox, mp->msg_part[0], &msg))
        continue;

      mu_message_get_body (msg, &body);

      mu_body_get_streamref (body, &stream);
      rc = mu_stream_copy (output, stream, 0, NULL);
      mu_stream_destroy (&stream);

      if (rc == 0)
	{
	  mu_body_size (body, &size);
	  total_size += size;
	  mu_body_lines (body, &size);
	  total_lines += size;
	  
	  /* mark as saved. */
	  
	  mu_message_get_attribute (msg, &attr);
	  mu_attribute_set_userflag (attr, MAIL_ATTRIBUTE_SAVED);
	}
      else
	mu_error (_("cannot save %lu: %s"),
		  (unsigned long) mp->msg_part[0], mu_strerror (rc));
    }

  mu_stream_close (output);
  mu_stream_destroy (&output);
  
  mu_printf ("\"%s\" %3lu/%-5lu\n", filename,
		    (unsigned long) total_lines, (unsigned long) total_size);

  free (filename);
  msgset_free (msglist);
  return 0;
}
