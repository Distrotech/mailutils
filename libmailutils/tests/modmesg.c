/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2013 Free Software Foundation, Inc.

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

#include <assert.h>
#include <mailutils/mailutils.h>

char *text = "From: root\n\
\n\
This is a test message.\n\
oo\n\
";

int
main (int argc, char **argv)
{
  int i;
  char *p;
  mu_message_t msg;
  mu_stream_t stream = NULL;
  mu_header_t hdr;
  mu_body_t body;
  char *buf = NULL;
  
  mu_set_program_name (argv[0]);

  mu_static_memory_stream_create (&stream, text, strlen (text));
  assert (mu_stream_to_message (stream, &msg) == 0);
  mu_stream_unref (stream);
  assert (mu_message_get_header (msg, &hdr) == 0);
  assert (mu_message_get_body (msg, &body) == 0);
  assert (mu_body_get_streamref (body, &stream) == 0);
  assert (mu_stream_seek (stream, 0, MU_SEEK_END, NULL) == 0);
  
  for (i = 1; i < argc; i++)
    {
      if (strcmp (argv[i], "-h") == 0)
	{
	  mu_printf ("usage: %s [-a HDR:VAL] [-t TEXT]\n", mu_program_name);
	  return 0;
	}
      
      if (strcmp (argv[i], "-a") == 0)
	{
	  i++;
	  assert (argv[i] != NULL);
	  p = strchr (argv[i], ':');
	  assert (p != NULL);
	  *p++ = 0;
	  while (*p && mu_isspace (*p))
	    p++;
	  assert (mu_header_set_value (hdr, argv[i], p, 1) == 0);
	}
      else if (strcmp (argv[i], "-l") == 0)
	{
	  mu_off_t off;
	  int whence = MU_SEEK_SET;
	  
	  i++;
	  assert (argv[i] != NULL);
	  off = strtol (argv[i], &p, 10);
	  assert (*p == 0);
	  if (off < 0)
	    whence = MU_SEEK_END;
	  assert (mu_stream_seek (stream, off, whence, NULL) == 0);
	}
      else if (strcmp (argv[i], "-t") == 0)
	{
	  size_t len;
	  i++;
	  assert (argv[i] != NULL);
	  len = strlen (argv[i]);
	  buf = realloc (buf, len + 1);
	  mu_wordsplit_c_unquote_copy (buf, argv[i], len);
	  assert (buf != NULL);
	  assert (mu_stream_write (stream, buf,
				   strlen (buf), NULL) == 0);
	}
      else
	mu_error ("ignoring unknown argument %s", argv[i]);
    }
  mu_stream_unref (stream);

  assert (mu_message_get_streamref (msg, &stream) == 0);
  assert (mu_stream_copy (mu_strout, stream, 0, NULL) == 0);
  mu_stream_unref (stream);

  return 0;
}

  
