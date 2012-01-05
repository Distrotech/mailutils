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

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <mailutils/mailutils.h>

int verbose;

void
ioloop (char *id, mu_stream_t in, mu_stream_t out)
{
  char *buf = NULL;
  size_t size = 0, n;
  int rc;
  
  while ((rc = mu_stream_getline (in, &buf, &size, &n)) == 0 && n > 0)
    {
      if (rc)
	{
	  mu_error("%s: read error: %s", id, mu_stream_strerror (in, rc));
	  exit (1);
	}
      MU_ASSERT (mu_stream_write (out, buf, n, NULL));
    }
  mu_stream_flush (out);
  if (verbose)
    fprintf (stderr, "%s exited\n", id);
}

int
main (int argc, char * argv [])
{
  mu_stream_t in, out, sock;
  pid_t pid;
  int status, c;

  while ((c = getopt (argc, argv, "v")) != EOF)
    switch (c)
      {
      case 'v':
	verbose++;
	break;

      case 'h':
	printf ("usage: musocio file\n");
	return 0;

      default:
	return 1;
      }

  argc -= optind;
  argv += optind;
  
  if (argc != 2)
    {
      mu_error ("usage: musocio file");
      return 1;
    }

  MU_ASSERT (mu_stdio_stream_create (&in, MU_STDIN_FD, 0));
  mu_stream_set_buffer (in, mu_buffer_line, 0);
  MU_ASSERT (mu_stdio_stream_create (&out, MU_STDOUT_FD, 0));
  mu_stream_set_buffer (out, mu_buffer_line, 0);
  MU_ASSERT (mu_socket_stream_create (&sock, argv[1], MU_STREAM_RDWR));
  mu_stream_set_buffer (sock, mu_buffer_line, 0);
  
  pid = fork ();
  if (pid == -1)
    {
      mu_error ("fork failed: %s", mu_strerror (errno));
      return 1;
    }

  if (pid == 0)
    {
      mu_stream_close (in);
      mu_stream_destroy (&in);
      ioloop ("reader", sock, out);
      exit (0);
    }

  ioloop ("writer", in, sock);

  mu_stream_close (in);
  mu_stream_destroy (&in);

  mu_stream_shutdown (sock, MU_STREAM_WRITE);
  waitpid (pid, &status, 0);

  mu_stream_close (sock);
  mu_stream_destroy (&sock);
  
  mu_stream_close (out);
  mu_stream_destroy (&out);
  return 0;
}
