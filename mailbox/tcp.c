/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2004, 2006, 2007, 2010 Free Software
   Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library; if not, write to the
   Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301 USA */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>

#include <netinet/in.h>

#include <arpa/inet.h>

#include <mailutils/errno.h>
#include <mailutils/stream.h>
#include <mailutils/mutil.h>

#include <mailutils/sys/stream.h>

#define TCP_STATE_INIT 		1
#define TCP_STATE_RESOLVE	2
#define TCP_STATE_RESOLVING	3
#define TCP_STATE_CONNECTING 	4
#define TCP_STATE_CONNECTED	5

struct _tcp_instance
{
  struct _mu_stream stream;
  int 		fd;
  char 		*host;
  int 		port;
  int		state;
  unsigned long	address;
  unsigned long source_addr;
};

/* On solaris inet_addr() return -1.  */
#ifndef INADDR_NONE
# define INADDR_NONE (unsigned long)-1
#endif

static int
_tcp_close (mu_stream_t stream)
{
  struct _tcp_instance *tcp = (struct _tcp_instance *)stream;
  int err = 0;

  if (tcp->fd != -1)
    {
      if (close (tcp->fd) != 0)
	{
	  err = errno;
	}
    }
  tcp->fd = -1;
  tcp->state = TCP_STATE_INIT;
  return err;
}

static int
resolve_hostname (const char *host, unsigned long *ip)
{
  unsigned long address = inet_addr (host);
  if (address == INADDR_NONE)
    {
      struct hostent *phe = gethostbyname (host);
      if (!phe)
	return MU_ERR_GETHOSTBYNAME;
      address = *(((unsigned long **) phe->h_addr_list)[0]);
    }
  *ip = address;
  return 0;
}

static int
_tcp_open (mu_stream_t stream)
{
  struct _tcp_instance *tcp = (struct _tcp_instance *)stream;
  int flgs, ret;
  socklen_t namelen;
  struct sockaddr_in peer_addr;
  struct sockaddr_in soc_addr;
  int flags;

  mu_stream_get_flags (stream, &flags);

  switch (tcp->state)
    {
    case TCP_STATE_INIT:
      if (tcp->fd == -1)
	{
	  if ((tcp->fd = socket (PF_INET, SOCK_STREAM, 0)) == -1)
	    return errno;
	}
      if (flags & MU_STREAM_NONBLOCK)
	{
	  flgs = fcntl (tcp->fd, F_GETFL);
	  flgs |= O_NONBLOCK;
	  fcntl (tcp->fd, F_SETFL, flgs);
	  mu_stream_set_flags (stream, MU_STREAM_NONBLOCK);
	}
      if (tcp->source_addr != INADDR_ANY)
	{
	  struct sockaddr_in s;
	  s.sin_family = AF_INET;
	  s.sin_addr.s_addr = tcp->source_addr;
	  s.sin_port = 0;
	  if (bind (tcp->fd, (struct sockaddr*) &s, sizeof(s)) < 0)
	    {
	      int e = errno;
	      close (tcp->fd);
	      tcp->fd = -1;
	      return e;
	    }
	}
      
      tcp->state = TCP_STATE_RESOLVING;
      
    case TCP_STATE_RESOLVING:
      if (!(tcp->host != NULL && tcp->port > 0))
	{
	  _tcp_close (stream);
	  return EINVAL;
	}
      
      if ((ret = resolve_hostname (tcp->host, &tcp->address)))
	{
	  _tcp_close (stream);
	  return ret;
	}
      tcp->state = TCP_STATE_RESOLVE;
      
    case TCP_STATE_RESOLVE:
      memset (&soc_addr, 0, sizeof (soc_addr));
      soc_addr.sin_family = AF_INET;
      soc_addr.sin_port = htons (tcp->port);
      soc_addr.sin_addr.s_addr = tcp->address;

      if ((connect (tcp->fd,
		    (struct sockaddr *) &soc_addr, sizeof (soc_addr))) == -1)
	{
	  ret = errno;
	  if (ret == EINPROGRESS || ret == EAGAIN)
	    {
	      tcp->state = TCP_STATE_CONNECTING;
	      ret = EAGAIN;
	    }
	  else
	    _tcp_close (stream);
	  return ret;
	}
      tcp->state = TCP_STATE_CONNECTING;
      
    case TCP_STATE_CONNECTING:
      namelen = sizeof (peer_addr);
      if (getpeername (tcp->fd,
		       (struct sockaddr *) &peer_addr, &namelen) == 0)
	tcp->state = TCP_STATE_CONNECTED;
      else
	{
	  ret = errno;
	  _tcp_close (stream);
	  return ret;
	}
      break;
    }
  return 0;
}

static int
_tcp_ioctl (mu_stream_t stream, int code, void *ptr)
{
  struct _tcp_instance *tcp = (struct _tcp_instance *)stream;
  mu_transport_t *ptrans;

  switch (code)
    {
    case MU_IOCTL_GET_TRANSPORT:
      if (!ptr)
	return EINVAL;
      ptrans = ptr;
      ptrans[0] = (mu_transport_t) tcp->fd;
      ptrans[1] = NULL;
      break;

    default:
      return ENOSYS;
    }
  return 0;
}

static int
_tcp_read (mu_stream_t stream, char *buf, size_t size, size_t *pret)
{
  struct _tcp_instance *tcp = (struct _tcp_instance *)stream;
  ssize_t bytes;

  if ((bytes = recv (tcp->fd, buf, size, 0)) == -1)
    return errno;
  *pret = bytes;
  return 0;
}

static int
_tcp_write (mu_stream_t stream, const char *buf, size_t size, size_t *pret)
{
  struct _tcp_instance *tcp = (struct _tcp_instance *)stream;
  ssize_t bytes;

  if ((bytes = send (tcp->fd, buf, size, 0)) == -1)
    return errno;
  *pret = bytes;
  return 0;
}

static void
_tcp_done (mu_stream_t stream)
{
  struct _tcp_instance *tcp = (struct _tcp_instance *)stream;

  if (tcp->host)
    free (tcp->host);
  if (tcp->fd != -1)
    close (tcp->fd);
}

int
_tcp_wait (mu_stream_t stream, int *pflags, struct timeval *tvp)
{
  struct _tcp_instance *tcp = (struct _tcp_instance *)stream;
  if (tcp->fd == -1)
    return EINVAL;
  return mu_fd_wait (tcp->fd, pflags, tvp);
}

int
_tcp_shutdown (mu_stream_t stream, int how)
{
  struct _tcp_instance *tcp = (struct _tcp_instance *)stream;
  int flag;
  if (tcp->fd == -1)
    return EINVAL;

  switch (how)
    {
    case MU_STREAM_READ:
      flag = SHUT_RD;
      break;
      
    case MU_STREAM_WRITE:
      flag = SHUT_WR;
    }

  if (shutdown (tcp->fd, flag))
    return errno;
  return 0;
}

static struct _tcp_instance *
_create_tcp_stream (int flags)
{
  struct _tcp_instance *tcp =
    (struct _tcp_instance *)_mu_stream_create (sizeof (*tcp), flags);

  if (tcp)
    {
      tcp->stream.open = _tcp_open;
      tcp->stream.close = _tcp_close;
      tcp->stream.read = _tcp_read;
      tcp->stream.write = _tcp_write;
      tcp->stream.ctl = _tcp_ioctl;
      tcp->stream.done = _tcp_done;
      tcp->stream.wait = _tcp_wait;
      tcp->stream.shutdown = _tcp_shutdown;
      tcp->fd = -1;
      tcp->state = TCP_STATE_INIT;
    }
  return tcp;
}

int
mu_tcp_stream_create_with_source_ip (mu_stream_t *stream,
				     const char *host, int port,
				     unsigned long source_ip,
				     int flags)
{
  struct _tcp_instance *tcp;

  if (host == NULL)
    return MU_ERR_TCP_NO_HOST;

  if (port < 1)
    return MU_ERR_TCP_NO_PORT;

  tcp = _create_tcp_stream (flags | MU_STREAM_RDWR);
  if (!tcp)
    return ENOMEM;
  tcp->host = strdup (host);
  if (!tcp->host)
    {
      free (tcp);
      return ENOMEM;
    }
  tcp->port = port;
  tcp->state = TCP_STATE_INIT;
  tcp->source_addr = source_ip;
  *stream = (mu_stream_t) tcp;
  return 0;
}

int
mu_tcp_stream_create_with_source_host (mu_stream_t *stream,
				       const char *host, int port,
				       const char *source_host,
				       int flags)
{
  unsigned long source_addr;
  int ret = resolve_hostname (source_host, &source_addr);
  if (ret == 0)
    ret = mu_tcp_stream_create_with_source_ip (stream, host, port,
					       source_addr, flags);
  return ret;
}
       
int
mu_tcp_stream_create (mu_stream_t *stream, const char *host, int port,
		      int flags)
{
  return mu_tcp_stream_create_with_source_ip (stream, host, port,
					      INADDR_ANY, flags);
}
