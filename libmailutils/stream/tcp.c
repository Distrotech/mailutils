/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2000, 2004, 2006-2007, 2010-2012 Free Software
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
   Public License along with this library.  If not, see 
   <http://www.gnu.org/licenses/>. */

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
#include <limits.h>

#include <sys/socket.h>
#include <sys/types.h>

#include <netinet/in.h>

#include <arpa/inet.h>

#include <mailutils/errno.h>
#include <mailutils/stream.h>
#include <mailutils/util.h>
#include <mailutils/sockaddr.h>
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
  int		state;
  struct mu_sockaddr *remote_addr;
  struct mu_sockaddr *source_addr;
};

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
_tcp_open (mu_stream_t stream)
{
  struct _tcp_instance *tcp = (struct _tcp_instance *)stream;
  int flgs, ret;
  socklen_t namelen;
  struct sockaddr_in peer_addr;
  int flags;

  mu_stream_get_flags (stream, &flags);

  switch (tcp->state)
    {
    case TCP_STATE_INIT:
      if (tcp->fd == -1)
	{
	  tcp->fd = socket (tcp->remote_addr->addr->sa_family, SOCK_STREAM, 0);
	  if (tcp->fd == -1)
	    return errno;
	}
      if (flags & MU_STREAM_NONBLOCK)
	{
	  flgs = fcntl (tcp->fd, F_GETFL);
	  flgs |= O_NONBLOCK;
	  fcntl (tcp->fd, F_SETFL, flgs);
	  mu_stream_set_flags (stream, MU_STREAM_NONBLOCK);
	}
      if (tcp->source_addr)
	{
	  if (bind (tcp->fd, tcp->source_addr->addr,
		    tcp->source_addr->addrlen) < 0)
	    {
	      int e = errno;
	      close (tcp->fd);
	      tcp->fd = -1;
	      return e;
	    }
	}
      
      tcp->state = TCP_STATE_RESOLVING;
      
    case TCP_STATE_RESOLVING:
      tcp->state = TCP_STATE_RESOLVE;
      
    case TCP_STATE_RESOLVE:
      if (connect (tcp->fd, tcp->remote_addr->addr,
		   tcp->remote_addr->addrlen) == -1)
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
_tcp_ioctl (mu_stream_t stream, int code, int opcode, void *ptr)
{
  struct _tcp_instance *tcp = (struct _tcp_instance *)stream;

  switch (code)
    {
    case MU_IOCTL_TRANSPORT:
      if (!ptr)
	return EINVAL;
      else
	{
	  mu_transport_t *ptrans = ptr;
	  switch (opcode)
	    {
	    case MU_IOCTL_OP_GET:
	      ptrans[0] = (mu_transport_t) tcp->fd;
	      ptrans[1] = NULL;
	      break;
	    case MU_IOCTL_OP_SET:
	      return ENOSYS;
	    default:
	      return EINVAL;
	    }
	}
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

  mu_sockaddr_free (tcp->remote_addr);
  mu_sockaddr_free (tcp->source_addr);
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
mu_tcp_stream_create_from_sa (mu_stream_t *pstream,
			      struct mu_sockaddr *remote_addr,
			      struct mu_sockaddr *source_addr, int flags)
{
  int rc;
  mu_stream_t stream;
  struct _tcp_instance *tcp;

  tcp = _create_tcp_stream (flags | MU_STREAM_RDWR);
  if (!tcp)
    return ENOMEM;

  tcp->remote_addr = remote_addr;
  tcp->source_addr = source_addr;
  
  stream = (mu_stream_t) tcp;
  rc = mu_stream_open (stream);
  if (rc == 0 || rc == EAGAIN || rc == EINPROGRESS)
    *pstream = stream;
  else
    {
      /* Make sure sockaddrs are not freed on error */
      tcp->remote_addr = tcp->source_addr = NULL;
      mu_stream_destroy (&stream);
    }
  return rc;
}


int
mu_tcp_stream_create_with_source_ip (mu_stream_t *pstream,
				     const char *host, unsigned port,
				     unsigned long source_ip,
				     int flags)
{
  int rc;
  struct mu_sockaddr *remote_addr, *source_addr = NULL;
  struct mu_sockaddr_hints hints;
  
  memset (&hints, 0, sizeof hints);
  hints.family = AF_INET;
  hints.socktype = SOCK_STREAM;
  hints.protocol = IPPROTO_TCP;
  hints.port = port;
  rc = mu_sockaddr_from_node (&remote_addr, host, NULL, &hints);
  if (rc)
    return rc;

  if (source_ip)
    {
      struct sockaddr_in s;
      s.sin_family = AF_INET;
      s.sin_addr.s_addr = source_ip;
      s.sin_port = 0;
      rc = mu_sockaddr_create (&source_addr, (struct sockaddr*)&s,
			       sizeof (s));
      if (rc)
	{
	  mu_sockaddr_free (remote_addr);
	  return 0;
	}
    }

  rc = mu_tcp_stream_create_from_sa (pstream, remote_addr, source_addr, flags);
  if (rc && !(rc == EAGAIN || rc == EINPROGRESS))
    {
      mu_sockaddr_free (remote_addr);
      mu_sockaddr_free (source_addr);
    }
  return rc;
}

int
mu_tcp_stream_create_with_source_host (mu_stream_t *stream,
				       const char *host, unsigned port,
				       const char *source_host,
				       int flags)
{
  int rc;
  struct mu_sockaddr *remote_addr, *source_addr = NULL;
  struct mu_sockaddr_hints hints;
  
  memset (&hints, 0, sizeof hints);
  hints.family = AF_INET;
  hints.socktype = SOCK_STREAM;
  hints.port = port;
  rc = mu_sockaddr_from_node (&remote_addr, host, NULL, &hints);
  if (rc)
    return rc;

  if (source_host)
    {
      hints.flags = MU_AH_PASSIVE;
      hints.port = 0;
      rc = mu_sockaddr_from_node (&source_addr, source_host, NULL, &hints);
      if (rc)
	{
	  mu_sockaddr_free (remote_addr);
	  return 0;
	}
    }

  rc = mu_tcp_stream_create_from_sa (stream, remote_addr, source_addr, flags);
  if (rc && !(rc == EAGAIN || rc == EINPROGRESS))
    {
      mu_sockaddr_free (remote_addr);
      mu_sockaddr_free (source_addr);
    }
  return rc;
}
       
int
mu_tcp_stream_create (mu_stream_t *stream, const char *host, unsigned port,
		      int flags)
{
  return mu_tcp_stream_create_with_source_host (stream, host, port,
						NULL, flags);
}
