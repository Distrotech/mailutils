/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2004-2005, 2007, 2010-2012 Free Software Foundation,
   Inc.

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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <mailutils/mailutils.h>

int
main (int argc, char **argv)
{
  int i;
  mu_stream_t in, out;
  mu_stream_t cvt;
  const char *args[5] = { "iconv" };
  
  if (argc < 3 || argc > 4)
    {
      fprintf (stderr, "usage: %s from-code to-code [err]\n", argv[0]);
      return 1;
    }

  MU_ASSERT (mu_stdio_stream_create (&in, MU_STDIN_FD, 0));
  args[1] = argv[1];
  args[2] = argv[2];
  i = 3;
  if (argc == 4)
    args[i++] = argv[3];
  args[i] = NULL;
  
  MU_ASSERT (mu_filter_create_args (&cvt, in, args[0], i, args,
				    MU_FILTER_DECODE,
				    MU_FILTER_READ));
  mu_stream_unref (in);
  MU_ASSERT (mu_stdio_stream_create (&out, MU_STDOUT_FD, 0));
  MU_ASSERT (mu_stream_copy (out, cvt, 0, NULL));
  mu_stream_unref (cvt);
  mu_stream_flush (out);
  return 0;
}
