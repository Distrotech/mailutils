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
#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <mailutils/mailutils.h>

int
main (int argc, char * argv [])
{
  int c;
  mu_stream_t in, out;
  int reread_option = 0;
  mu_off_t reread_off;
  int skip_option = 0;
  mu_off_t skip_off;
  
  while ((c = getopt (argc, argv, "hs:r:")) != EOF)
    switch (c)
      {
      case 'r':
	reread_option = 1;
	reread_off = strtoul (optarg, NULL, 10);
	break;

      case 's':
	skip_option = 1;
	skip_off = strtoul (optarg, NULL, 10);
	break;

      case 'h':
	printf ("usage: cat [-s off]\n");
	exit (0);

      default:
	exit (1);
      }
	

  MU_ASSERT (mu_stdio_stream_create (&in, MU_STDIN_FD, MU_STREAM_SEEK));
  MU_ASSERT (mu_stdio_stream_create (&out, MU_STDOUT_FD, 0));

  if (skip_option)
    {
      mu_stream_printf (out, "skipping to %lu:\n",
			(unsigned long) skip_off);
      MU_ASSERT (mu_stream_seek (in, skip_off, MU_SEEK_SET, NULL));
    }
  
  MU_ASSERT (mu_stream_copy (out, in, 0, NULL));

  if (reread_option)
    {
      mu_stream_printf (out, "rereading from %lu:\n",
			(unsigned long) reread_off);
      MU_ASSERT (mu_stream_seek (in, reread_off, MU_SEEK_SET, NULL));
      MU_ASSERT (mu_stream_copy (out, in, 0, NULL));
    }
  
  mu_stream_close (in);
  mu_stream_destroy (&in);
  mu_stream_close (out);
  mu_stream_destroy (&out);
  return 0;
}
