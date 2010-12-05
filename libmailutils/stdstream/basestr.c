/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2010 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>. */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <mailutils/types.h>
#include <mailutils/errno.h>
#include <mailutils/log.h>
#include <mailutils/stream.h>
#include <mailutils/stdstream.h>

mu_stream_t mu_strin;
mu_stream_t mu_strout;
mu_stream_t mu_strerr;

void
mu_stdstream_setup ()
{
  int rc;
  int fd;

  /* If the streams are already open, close them */
  mu_stream_destroy (&mu_strin);
  mu_stream_destroy (&mu_strout);
  mu_stream_destroy (&mu_strerr);
  
  /* Ensure that first 3 descriptors are open in proper mode */
  fd = open ("/dev/null", O_WRONLY);
  switch (fd)
    {
    case 2:
      /* keep it open */;
      break;

    case 1:
      /* keep it open and try 0 */
      fd = open ("/dev/null", O_RDONLY);
      if (fd != 0)
	close (fd);
      break;
      
    default:
      close (fd);
      break;
    }

  /* Create the corresponding streams */
  rc = mu_stdio_stream_create (&mu_strin, MU_STDIN_FD, 0);
  if (rc)
    {
      fprintf (stderr, "mu_stdio_stream_create(%d): %s\n",
	       MU_STDIN_FD, mu_strerror (rc));
      abort ();
    }
  rc = mu_stdio_stream_create (&mu_strout, MU_STDOUT_FD, 0);
  if (rc)
    {
      fprintf (stderr, "mu_stdio_stream_create(%d): %s\n",
	       MU_STDOUT_FD, mu_strerror (rc));
      abort ();
    }

  if (mu_stdstream_strerr_create (&mu_strerr, MU_STRERR_STDERR, 0, 0,
				  mu_program_name, NULL))
    abort ();
  /* FIXME: atexit (flushall) */
}

int
mu_printf (const char *fmt, ...)
{
  int rc;
  va_list ap;

  va_start (ap, fmt);
  rc = mu_stream_vprintf (mu_strout, fmt, ap);
  va_end (ap);
  return rc;
}
