/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2011-2012 Free Software Foundation, Inc.

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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <mailutils/imapio.h>
#include <mailutils/errno.h>
#include <mailutils/error.h>
#include <mailutils/stream.h>
#include <mailutils/stdstream.h>

void
usage ()
{
  mu_stream_printf (mu_strout, "usage: %s [debug=SPEC] [-transcript] [-server]\n",
		    mu_program_name);
  exit (0);
}

int
main (int argc, char **argv)
{
  int i, rc;
  int transcript = 0;
  mu_imapio_t io;
  mu_stream_t str;
  int imapio_mode = MU_IMAPIO_CLIENT;
  
  mu_stdstream_setup (MU_STDSTREAM_RESET_NONE);
  
  for (i = 1; i < argc; i++)
    {
      char *opt = argv[i];

      if (strncmp (opt, "debug=", 6) == 0)
	mu_debug_parse_spec (opt + 6);
      else if (strcmp (opt, "-transcript") == 0)
	transcript = 1;
      else if (strcmp (opt, "-server") == 0)
	imapio_mode = MU_IMAPIO_SERVER;
      else if (strcmp (opt, "-h") == 0)
	usage ();
      else
	{
	  mu_error ("%s: unrecognized argument", opt);
	  exit (1);
	}
    }

  MU_ASSERT (mu_iostream_create (&str, mu_strin, mu_strout));

  
  MU_ASSERT (mu_imapio_create (&io, str, imapio_mode));

  if (transcript)
    mu_imapio_trace_enable (io);
  mu_stream_unref (str);

  while ((rc = mu_imapio_getline (io)) == 0)
    {
      size_t wc;
      char **wv;

      MU_ASSERT (mu_imapio_get_words (io, &wc, &wv));
      if (wc == 0)
	break;

      mu_stream_printf (mu_strerr, "%lu\n", (unsigned long) wc);
      for (i = 0; i < wc; i++)
	{
	  mu_stream_printf (mu_strerr, "%d: '%s'\n", i, wv[i]);
	}
    }

  if (rc)
    mu_error ("mu_imap_getline: %s", mu_strerror (rc));

  mu_imapio_free (io);
  return 0;
}
