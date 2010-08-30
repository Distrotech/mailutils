/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002, 2004, 
   2005, 2006, 2007, 2008, 2009, 2010 Free Software Foundation, Inc.

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
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include <mailutils/types.h>
#include <mailutils/alloc.h>
#include <mailutils/error.h>
#include <mailutils/errno.h>
#include <mailutils/nls.h>
#include <mailutils/stream.h>
#include <mailutils/sys/socket_stream.h>

static void
_socket_done (mu_stream_t stream)
{
  struct _mu_socket_stream *s = (struct _mu_socket_stream *) stream;

  if (s->filename)
    free (s->filename);
}

static int
_socket_open (mu_stream_t stream)
{
  struct _mu_socket_stream *s = (struct _mu_socket_stream *) stream;
  int fd, rc;
  struct sockaddr_un addr;
  
  if (!s)
    return EINVAL;
  
  fd = socket (PF_UNIX, SOCK_STREAM, 0);
  if (fd < 0)
    return errno;
  
  memset (&addr, 0, sizeof addr);
  addr.sun_family = AF_UNIX;
  strncpy (addr.sun_path, s->filename, sizeof addr.sun_path - 1);
  addr.sun_path[sizeof addr.sun_path - 1] = 0;
  if (connect (fd, (struct sockaddr *) &addr, sizeof(addr)))
    {
      close (fd);
      return errno;
    }

  s->stdio_stream.file_stream.fd = fd;
  s->stdio_stream.size = 0;
  s->stdio_stream.offset = 0;
  if (s->stdio_stream.cache)
    mu_stream_truncate (s->stdio_stream.cache, 0);
      
  return rc;
}

static int
_socket_close (mu_stream_t stream)
{
  struct _mu_socket_stream *s = (struct _mu_socket_stream *) stream;
  if (s->stdio_stream.file_stream.fd != -1)
    {
      close (s->stdio_stream.file_stream.fd);
      s->stdio_stream.file_stream.fd = -1;
    }
  return 0;
}

int
_socket_shutdown (mu_stream_t stream, int how)
{
  struct _mu_socket_stream *s = (struct _mu_socket_stream *) stream;
  int flag;

  switch (how)
    {
    case MU_STREAM_READ:
      flag = SHUT_RD;
      break;
      
    case MU_STREAM_WRITE:
      flag = SHUT_WR;
    }
  
  if (shutdown (s->stdio_stream.file_stream.fd, flag))
    return errno;
  return 0;
}

int
mu_socket_stream_create (mu_stream_t *pstream, const char *filename, int flags)
{
  struct _mu_socket_stream *s;
  int rc;
  
  rc = _mu_stdio_stream_create (pstream, sizeof (*s),
                                flags | MU_STREAM_AUTOCLOSE);
  if (rc)
    return rc;
  s = (struct _mu_socket_stream *) *pstream;
  s->stdio_stream.file_stream.stream.done = _socket_done;
  s->stdio_stream.file_stream.stream.open = _socket_open;
  s->stdio_stream.file_stream.stream.close = _socket_close;
  s->stdio_stream.file_stream.stream.shutdown = _socket_shutdown;
  return 0;
}
