/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2002, 2004-2012 Free Software Foundation, Inc.

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
#include <mailutils/sys/file_stream.h>

static int
_socket_open (mu_stream_t stream)
{
  struct _mu_file_stream *s = (struct _mu_file_stream *) stream;
  int fd;
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

  s->fd = fd;
      
  return 0;
}

int
_socket_shutdown (mu_stream_t stream, int how)
{
  struct _mu_file_stream *s = (struct _mu_file_stream *) stream;
  int flag;

  switch (how)
    {
    case MU_STREAM_READ:
      flag = SHUT_RD;
      break;
      
    case MU_STREAM_WRITE:
      flag = SHUT_WR;
    }
  
  if (shutdown (s->fd, flag))
    return errno;
  return 0;
}

int
mu_socket_stream_create (mu_stream_t *pstream, const char *filename, int flags)
{
  int rc;
  mu_stream_t transport;
  int need_cache;
  struct _mu_file_stream *fstr;
  
  need_cache = flags & MU_STREAM_SEEK;
  if (need_cache && (flags & MU_STREAM_WRITE))
    /* Write caches are not supported */
    return EINVAL;

  /* Create transport stream. */
  rc = _mu_file_stream_create (&fstr, sizeof (*fstr),
			       filename, -1,
			       flags & ~MU_STREAM_SEEK);
  if (rc)
    return rc;
  fstr->stream.open = _socket_open;
  fstr->stream.shutdown = _socket_shutdown;
  transport = (mu_stream_t) fstr;
  
  /* Wrap it in cache, if required */
  if (need_cache)
    {
      mu_stream_t str;
      rc = mu_rdcache_stream_create (&str, transport, flags);
      mu_stream_unref (transport);
      if (rc)
	return rc;
      transport = str;
    }

  rc = mu_stream_open (transport);
  if (rc)
    mu_stream_unref (transport);
  else
    *pstream = transport;
  return rc;
}
