/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2002, 2005, 2007, 2010-2012 Free Software
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <unistd.h>
#include <stdio.h>
#include <mailutils/mailutils.h>

const char *name;
mu_stream_t stream;

void
match_string (const char *str)
{
  int rc;
  mu_url_t u, url;
  struct mu_locus loc;
  
  if ((rc = mu_url_create (&u, str)) != 0)
    {
      fprintf (stderr, "mu_url_create %s ERROR: [%d] %s",
	       str, rc, mu_strerror (rc));
      return;
    }
  MU_ASSERT (mu_stream_seek (stream, 0, MU_SEEK_SET, NULL));
  loc.mu_file = (char*)name;
  loc.mu_line = 0;
  loc.mu_col = 0;
  rc = mu_wicket_stream_match_url (stream, &loc, u, MU_URL_PARSE_ALL, &url);
  switch (rc)
    {
    case 0:
      printf ("%s matches %s at %s:%d\n",
	      mu_url_to_string (u), mu_url_to_string (url),
	      loc.mu_file, loc.mu_line);
      mu_url_destroy (&url);
      break;

    case MU_ERR_NOENT:
      printf ("no matches for %s\n", mu_url_to_string (u));
      break;

    default:
      mu_error ("mu_wicket_stream_match_url: %s", mu_strerror (rc));
      exit (1);
    }
  mu_url_destroy (&u);
}


int
main (int argc, char **argv)
{
  if (argc < 2)
    {
      fprintf (stderr, "usage: %s filename [url [url...]]\n", argv[0]);
      return 1;
    }

  name = argv[1];
  MU_ASSERT (mu_file_stream_create (&stream, name, MU_STREAM_READ));

  if (argc > 2)
    {
      int i;

      for (i = 2; i < argc; i++)
	match_string (argv[i]);
    }
  else
    {
      mu_stream_t in;
      char *buf = NULL;
      size_t size = 0, n;
      int rc;
      
      MU_ASSERT (mu_stdio_stream_create (&in, MU_STDIN_FD, 0));

      while ((rc = mu_stream_getline (in, &buf, &size, &n)) == 0
	     && n > 0)
	match_string (mu_str_stripws (buf));
      free (buf);
      mu_stream_destroy (&in);
    }
  mu_stream_destroy (&stream);
  return 0;
}

	  

	
