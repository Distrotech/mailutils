/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* This is an example program to illustrate the use of stream functions.
   It connects to a remote HTTP server and prints the contents of its
   index page */
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <mailutils/mailutils.h>

const char *wbuf = "GET / HTTP/1.0\r\n\r\n";
char rbuf[1024];

int
main ()
{
  int ret, off = 0, fd;
  stream_t stream;
  size_t nb;
  fd_set fds;

  ret = tcp_stream_create (&stream, "www.gnu.org", 80, MU_STREAM_NONBLOCK);
  if (ret != 0)
    {
      mu_error ( "tcp_stream_create: %s\n", mu_errstring (ret));
      exit (EXIT_FAILURE);
    }

connect_again:
  ret = stream_open (stream);
  if (ret != 0)
    {
      if (ret == EAGAIN)
        {
          ret = stream_get_fd (stream, &fd);
          if (ret != 0)
            {
              mu_error ( "stream_get_fd: %s\n", mu_errstring (ret));
              exit (EXIT_FAILURE);
            }
          FD_ZERO (&fds);
          FD_SET (fd, &fds);
          select (fd + 1, NULL, &fds, NULL, NULL);
          goto connect_again;
        }
      mu_error ( "stream_open: %s\n", mu_errstring (ret));
      exit (EXIT_FAILURE);
    }

  ret = stream_get_fd (stream, &fd);
  if (ret != 0)
    {
      mu_error ( "stream_get_fd: %s\n", mu_errstring (ret));
      exit (EXIT_FAILURE);
    }

write_again:
  ret = stream_write (stream, wbuf + off, strlen (wbuf), 0, &nb);
  if (ret != 0)
    {
      if (ret == EAGAIN)
        {
          FD_ZERO (&fds);
          FD_SET (fd, &fds);
          select (fd + 1, NULL, &fds, NULL, NULL);
          off += nb;
          goto write_again;
        }
      mu_error ( "stream_write: %s\n", mu_errstring (ret));
      exit (EXIT_FAILURE);
    }

  if (nb != strlen (wbuf))
    {
      mu_error ( "stream_write: %s\n", "nb != wbuf length");
      exit (EXIT_FAILURE);
    }

  do
    {
      ret = stream_read (stream, rbuf, sizeof (rbuf), 0, &nb);
      if (ret != 0)
        {
          if (ret == EAGAIN)
            {
              FD_ZERO (&fds);
              FD_SET (fd, &fds);
              select (fd + 1, &fds, NULL, NULL, NULL);
            }
          else
            {
              mu_error ( "stream_read: %s\n", mu_errstring (ret));
              exit (EXIT_FAILURE);
            }
        }
      write (2, rbuf, nb);
    }
  while (nb || ret == EAGAIN);

  ret = stream_close (stream);
  if (ret != 0)
    {
      mu_error ( "stream_close: %s\n", mu_errstring (ret));
      exit (EXIT_FAILURE);
    }

  stream_destroy (&stream, NULL);
  exit (EXIT_SUCCESS);
}
