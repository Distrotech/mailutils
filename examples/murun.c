/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003 Free Software Foundation, Inc.

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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <mailutils/mailutils.h>
#include <mailutils/argcv.h>

static void
read_and_print (stream_t in, stream_t out)
{
  size_t size;
  char buffer[128];
  
  while (stream_sequential_readline (in, buffer, sizeof (buffer), &size) == 0
	 && size > 0)
    {
      stream_sequential_write (out, buffer, size);
    }
}

int
main (int argc, char *argv[])
{
  int rc;
  stream_t stream, out;
  int read_stdin = 0;
  int i = 1;
  char *cmdline;
  int flags = MU_STREAM_READ;

  if (argc > 1 && strcmp (argv[i], "--stdin") == 0)
    {
      read_stdin = 1;
      flags |= MU_STREAM_WRITE;
      i++;
    }
  
  if (i == argc)
    {
      fprintf (stderr, "Usage: %s [--stdin] progname [args]\n", argv[0]);
      exit (1);
    }

  assert (argcv_string (argc - i, &argv[i], &cmdline) == 0);
  if (read_stdin)
    {
      stream_t in;
      assert (stdio_stream_create (&in, stdin, 0) == 0);
      assert (stream_open (in) == 0);
      rc = filter_prog_stream_create (&stream, cmdline, in);
    }
  else
    rc = prog_stream_create (&stream, cmdline, flags);
  if (rc)
    {
      fprintf (stderr, "%s: cannot create program filter stream: %s\n",
	       argv[0], mu_strerror (rc));
      exit (1);
    }

  rc = stream_open (stream);
  if (rc)
    {
      fprintf (stderr, "%s: cannot open program filter stream: %s\n",
	       argv[0], mu_strerror (rc));
      exit (1);
    }

  assert (stdio_stream_create (&out, stdout, 0) == 0);
  assert (rc == 0);
  assert (stream_open (out) == 0);
  
  read_and_print (stream, out);
  stream_close (stream);
  stream_destroy (&stream, stream_get_owner (stream));
  stream_close (out);
  stream_destroy (&out, stream_get_owner (stream));
  return 0;
}
