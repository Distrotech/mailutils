/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU Library General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <netdb.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>

#include <mailutils/sys/tcpstream.h>
#include <mailutils/error.h>
#include <mailutils/monitor.h>

/* On solaris inet_addr() return -1.  */
#ifndef INADDR_NONE
# define INADDR_NONE (unsigned long)-1
#endif

static void
_tcp_cleanup (void *arg)
{
  struct _tcp_instance *tcp = arg;
  mu_refcount_unlock (tcp->refcount);
}

static int
_tcp_ref (stream_t stream)
{
  struct _tcp_instance *tcp = (struct _tcp_instance *)stream;
  return mu_refcount_inc (tcp->refcount);
}

static void
_tcp_destroy (stream_t *pstream)
{
  struct _tcp_instance *tcp = (struct _tcp_instance *)*pstream;
  if (mu_refcount_dec (tcp->refcount) == 0)
    {
      if (tcp->host)
	free (tcp->host);
      if (tcp->fd != -1)
	close (tcp->fd);
      mu_refcount_destroy (&tcp->refcount);
      free (tcp);
    }
}

static int
_tcp_close0 (stream_t stream)
{
  struct _tcp_instance *tcp = (struct _tcp_instance *)stream;
  if (tcp->fd != -1)
    close (tcp->fd);
  tcp->fd = -1;
  tcp->state = TCP_STATE_INIT;
  return 0;
}

static int
_tcp_close (stream_t stream)
{
  struct _tcp_instance *tcp = (struct _tcp_instance *)stream;

  mu_refcount_lock (tcp->refcount);
  monitor_cleanup_push (_tcp_cleanup, tcp);
  _tcp_close0 (stream);
  mu_refcount_unlock (tcp->refcount);
  monitor_cleanup_pop (0);
  return 0;
}

static int
_tcp_open0 (stream_t stream, const char *host, int port, int flags)
{
  struct _tcp_instance 	*tcp = (struct _tcp_instance *)stream;
  int flgs, ret;
  size_t namelen;
  struct sockaddr_in peer_addr;
  struct hostent *phe;
  struct sockaddr_in soc_addr;

  if (tcp->state == TCP_STATE_INIT)
    {
      tcp->port = port;
      tcp->host = strdup (host);
      if (tcp->host == NULL)
	return MU_ERROR_NO_MEMORY;
      tcp->flags = flags;
    }

  switch (tcp->state)
    {
    case TCP_STATE_INIT:
      if (tcp->fd == -1)
	{
	  tcp->fd = socket (AF_INET, SOCK_STREAM, 0);
	  if (tcp->fd == -1)
	    return errno;
	}
      if (tcp->flags & MU_STREAM_NONBLOCK)
	{
	  flgs = fcntl (tcp->fd, F_GETFL);
	  flgs |= O_NONBLOCK;
	  fcntl (tcp->fd, F_SETFL, flgs);
	}
      tcp->state = TCP_STATE_RESOLVING;

    case TCP_STATE_RESOLVING:
      if (tcp->host == NULL || tcp->port == -1)
	return MU_ERROR_INVALID_PARAMETER;
      tcp->address = inet_addr (tcp->host);
      if (tcp->address == INADDR_NONE)
	{
	  phe = gethostbyname (tcp->host);
	  if (!phe)
	    {
	      _tcp_close0 (stream);
	      return MU_ERROR_INVALID_PARAMETER;
	    }
	  tcp->address = *(((unsigned long **)phe->h_addr_list)[0]);
	}
      tcp->state = TCP_STATE_RESOLVE;

    case TCP_STATE_RESOLVE:
      memset (&soc_addr, 0, sizeof soc_addr);
      soc_addr.sin_family = AF_INET;
      soc_addr.sin_port = htons (tcp->port);
      soc_addr.sin_addr.s_addr = tcp->address;

      if ((connect (tcp->fd, (struct sockaddr *)&soc_addr, sizeof soc_addr)) == -1)
	{
	  ret = errno;
	  if (ret == EINPROGRESS || ret == EAGAIN)
	    {
	      tcp->state = TCP_STATE_CONNECTING;
	      ret = MU_ERROR_TRY_AGAIN;
	    }
	  else
	    _tcp_close0 (stream);
	  return ret;
	}
      tcp->state = TCP_STATE_CONNECTING;

    case TCP_STATE_CONNECTING:
      namelen = sizeof peer_addr;
      if (getpeername (tcp->fd, (struct sockaddr *)&peer_addr, &namelen) == 0)
	tcp->state = TCP_STATE_CONNECTED;
      else
	{
	  ret = errno;
	  _tcp_close0 (stream);
	  return ret;
	}
      break;
    }
  return 0;
}

static int
_tcp_open (stream_t stream, const char *host, int port, int flags)
{
  int status;
  struct _tcp_instance *tcp = (struct _tcp_instance *)stream;
  mu_refcount_lock (tcp->refcount);
  monitor_cleanup_push (_tcp_cleanup, tcp);
  status = _tcp_open0 (stream, host, port, flags);
  mu_refcount_unlock (tcp->refcount);
  monitor_cleanup_pop (0);
  return status;
}

static int
_tcp_get_fd (stream_t stream, int *fd)
{
  struct _tcp_instance *tcp = (struct _tcp_instance *)stream;

  if (fd == NULL || tcp->fd == -1)
    return MU_ERROR_INVALID_PARAMETER;

  *fd = tcp->fd;
  return 0;
}

static int
_tcp_read (stream_t stream, void *buf, size_t buf_size, size_t *br)
{
  struct _tcp_instance *tcp = (struct _tcp_instance *)stream;
  int bytes = 0;
  int status = 0;

  bytes = recv (tcp->fd, buf, buf_size, 0);
  if (bytes == -1)
    {
      bytes = 0;
      status = errno;
    }
  if (br)
    *br = bytes;
  return status;
}

static int
_tcp_readline (stream_t stream, char *buf, size_t buf_size, size_t *br)
{
  struct _tcp_instance *tcp = (struct _tcp_instance *)stream;
  int status = 0;
  size_t n;
  int nr = 0;
  char c;

  /* Grossly inefficient hopefully they override this */
  for (n = 1; n < buf_size; n++)
    {
      nr = recv (tcp->fd, &c, 1, 0);
      if (nr == -1) /* Error.  */
	{
	  status = errno;
	  break;
	}
      else if (nr == 1)
	{
	  *buf++ = c;
	  if (c == '\n') /* Newline is stored like fgets().  */
	    break;
	}
      else if (nr == 0)
	{
	  if (n == 1) /* EOF, no data read.  */
	    n = 0;
	  break; /* EOF, some data was read.  */
	}
    }
  *buf = '\0';
  if (br)
    *br = (n == buf_size) ? n - 1: n;
  return status;
}

static int
_tcp_write (stream_t stream, const void *buf, size_t buf_size, size_t *bw)
{
  struct _tcp_instance *tcp = (struct _tcp_instance *)stream;
  int bytes = 0;
  int status = 0;

  bytes = send (tcp->fd, buf, buf_size, 0);
  if (bytes == -1)
    {
      bytes = 0;
      status = errno;
    }
  if (bw)
    *bw = bytes;
  return status;
}

static int
_tcp_seek (stream_t stream, off_t off, enum stream_whence whence)
{
  (void)stream; (void)off; (void)whence;
  return MU_ERROR_NOT_SUPPORTED;
}

static int
_tcp_tell (stream_t stream, off_t *off)
{
  (void)stream; (void)off;
  return MU_ERROR_NOT_SUPPORTED;
}

static int
_tcp_get_size (stream_t stream, off_t *off)
{
  (void)stream; (void)off;
  return MU_ERROR_NOT_SUPPORTED;
}

static int
_tcp_truncate (stream_t stream, off_t off)
{
  (void)stream; (void)off;
  return MU_ERROR_NOT_SUPPORTED;
}

static int
_tcp_flush (stream_t stream)
{
  (void)stream;
  return 0;
}

static int
_tcp_get_flags (stream_t stream, int *flags)
{
  struct _tcp_instance *tcp = (struct _tcp_instance *)stream;
  if (flags == NULL)
    return MU_ERROR_INVALID_PARAMETER;
  *flags = tcp->flags;
  return 0;
}

static int
_tcp_get_state (stream_t stream, enum stream_state *state)
{
  struct _tcp_instance *tcp = (struct _tcp_instance *)stream;
  if (state == NULL)
    return MU_ERROR_INVALID_PARAMETER;
  *state = tcp->state;
  return 0;
}

static int
_tcp_is_readready (stream_t stream, int timeout)
{
  struct _tcp_instance *tcp = (struct _tcp_instance *)stream;
  int ready;
  struct timeval tv;
  fd_set fds;

  FD_ZERO (&fds);
  FD_SET (tcp->fd, &fds);

  tv.tv_sec  = timeout / 100;
  tv.tv_usec = (timeout % 1000) * 1000;

  ready = select (tcp->fd + 1, &fds, NULL, NULL, (timeout == -1) ? NULL: &tv);
  return (ready == -1) ? 0 : 1;
}

static int
_tcp_is_writeready (stream_t stream, int timeout)
{
  struct _tcp_instance *tcp = (struct _tcp_instance *)stream;
  int ready;
  struct timeval tv;
  fd_set fds;

  FD_ZERO (&fds);
  FD_SET (tcp->fd, &fds);

  tv.tv_sec  = timeout / 100;
  tv.tv_usec = (timeout % 1000) * 1000;

  ready = select (tcp->fd + 1, NULL, &fds, NULL, (timeout == -1) ? NULL: &tv);
  return (ready == -1) ? 0 : 1;
}

static int
_tcp_is_exceptionpending (stream_t stream, int timeout)
{
  struct _tcp_instance *tcp = (struct _tcp_instance *)stream;
  int ready;
  struct timeval tv;
  fd_set fds;

  FD_ZERO (&fds);
  FD_SET  (tcp->fd, &fds);

  tv.tv_sec  = timeout / 100;
  tv.tv_usec = (timeout % 1000) * 1000;

  ready = select (tcp->fd + 1, NULL, NULL, &fds, (timeout == -1) ? NULL: &tv);
  return (ready == -1) ? 0 : 1;
}

static int
_tcp_is_open (stream_t stream)
{
  struct _tcp_instance *tcp = (struct _tcp_instance *)stream;
  return tcp->fd >= 0;
}

static struct _stream_vtable _tcp_vtable =
{
  _tcp_ref,
  _tcp_destroy,

  _tcp_open,
  _tcp_close,

  _tcp_read,
  _tcp_readline,
  _tcp_write,

  _tcp_seek,
  _tcp_tell,

  _tcp_get_size,
  _tcp_truncate,
  _tcp_flush,

  _tcp_get_fd,
  _tcp_get_flags,
  _tcp_get_state,

  _tcp_is_readready,
  _tcp_is_writeready,
  _tcp_is_exceptionpending,

  _tcp_is_open
};

int
stream_tcp_create (stream_t *pstream)
{
  struct _tcp_instance *tcp;

  if (pstream == NULL)
    return MU_ERROR_INVALID_PARAMETER;

  tcp = calloc (1, sizeof *tcp);
  if (tcp == NULL)
    return MU_ERROR_NO_MEMORY;

  mu_refcount_create (&tcp->refcount);
  if (tcp->refcount == NULL)
    {
      free (tcp);
      return MU_ERROR_NO_MEMORY;
    }
  tcp->fd = -1;
  tcp->host = NULL;
  tcp->port = -1;
  tcp->state = TCP_STATE_INIT;
  tcp->base.vtable = &_tcp_vtable;
  *pstream = &tcp->base;
  return 0;
}
