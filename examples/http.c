/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2004 Free Software Foundation, Inc.

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

/* This is an example program to illustrate the use of stream functions.
   It connects to a remote HTTP server and prints the contents of its
   index page */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <mailutils/mailutils.h>

const char *wbuf = "GET / HTTP/1.0\r\n\r\n";
char rbuf[1024];

int
main (void)
{
  int ret, off = 0;
  stream_t stream;
  size_t nb;
  
  ret = tcp_stream_create (&stream, "www.gnu.org", 80, MU_STREAM_NONBLOCK);
  if (ret != 0)
    {
      mu_error ("tcp_stream_create: %s", mu_strerror (ret));
      exit (EXIT_FAILURE);
    }

connect_again:
  ret = stream_open (stream);
  if (ret != 0)
    {
      if (ret == EAGAIN)
        {
	  int wflags = MU_STREAM_READY_WR;
	  stream_wait (stream, &wflags, NULL);
          goto connect_again;
        }
      mu_error ("stream_open: %s", mu_strerror (ret));
      exit (EXIT_FAILURE);
    }

write_again:
  ret = stream_write (stream, wbuf + off, strlen (wbuf), 0, &nb);
  if (ret != 0)
    {
      if (ret == EAGAIN)
        {
	  int wflags = MU_STREAM_READY_WR;
	  stream_wait (stream, &wflags, NULL);
          off += nb;
          goto write_again;
        }
      mu_error ("stream_write: %s", mu_strerror (ret));
      exit (EXIT_FAILURE);
    }

  if (nb != strlen (wbuf))
    {
      mu_error ("stream_write: %s", "nb != wbuf length");
      exit (EXIT_FAILURE);
    }

  do
    {
      ret = stream_read (stream, rbuf, sizeof (rbuf), 0, &nb);
      if (ret != 0)
        {
          if (ret == EAGAIN)
            {
	      int wflags = MU_STREAM_READY_RD;
	      stream_wait (stream, &wflags, NULL);
            }
          else
            {
              mu_error ("stream_read: %s", mu_strerror (ret));
              exit (EXIT_FAILURE);
            }
        }
      write (1, rbuf, nb);
    }
  while (nb || ret == EAGAIN);

  ret = stream_close (stream);
  if (ret != 0)
    {
      mu_error ("stream_close: %s", mu_strerror (ret));
      exit (EXIT_FAILURE);
    }

  stream_destroy (&stream, NULL);
  exit (EXIT_SUCCESS);
}
