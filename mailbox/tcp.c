/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000 Free Software Foundation, Inc.

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

#include <assert.h>
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

#define TCP_STATE_INIT 		1
#define TCP_STATE_RESOLVE	2
#define TCP_STATE_RESOLVING	3
#define TCP_STATE_CONNECTING 	4
#define TCP_STATE_CONNECTED	5

struct _tcp_instance {
	int 		fd;
	char 		*host;
	int 		port;
	int		state;
	unsigned long	address;
};

/* On solaris inet_addr() return -1.  */
#ifndef INADDR_NONE
# define INADDR_NONE (unsigned long)-1
#endif

static int
_tcp_close (stream_t stream)
{
  struct _tcp_instance *tcp = stream_get_owner (stream);

  if (tcp->fd != -1)
    close (tcp->fd);
  tcp->fd = -1;
  tcp->state = TCP_STATE_INIT;
  return 0;
}

static int
_tcp_open (stream_t stream)
{
  struct _tcp_instance *tcp = stream_get_owner (stream);
  int flgs, ret;
  size_t namelen;
  struct sockaddr_in peer_addr;
  struct hostent *phe;
  struct sockaddr_in soc_addr;
  char* host = tcp->host;
  int port = tcp->port;
  int flags;

  stream_get_flags(stream, &flags);

  if (tcp->state == TCP_STATE_INIT)
    {
      tcp->port = port;
/* FIXME: this seems very strange, it is: tcp->host = strdup(tcp->host)
   is this really intended? */
      if ((tcp->host = strdup (host)) == NULL)
	return ENOMEM;
    }

  switch (tcp->state)
    {
    case TCP_STATE_INIT:
      if (tcp->fd == -1)
	{
	  if ((tcp->fd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
	    return errno;
	}
      if (flags & MU_STREAM_NONBLOCK)
	{
	  flgs = fcntl (tcp->fd, F_GETFL);
	  flgs |= O_NONBLOCK;
	  fcntl (tcp->fd, F_SETFL, flgs);
	  stream_set_flags (stream, MU_STREAM_NONBLOCK);
	}
      tcp->state = TCP_STATE_RESOLVING;
    case TCP_STATE_RESOLVING:
      assert (tcp->host != NULL && tcp->port > 0);
      tcp->address = inet_addr (tcp->host);
      if (tcp->address == INADDR_NONE)
	{
	  phe = gethostbyname (tcp->host);
	  if (!phe)
	    {
	      _tcp_close (stream);
	      return MU_ERR_GETHOSTBYNAME;
	    }
	  tcp->address = *(((unsigned long **) phe->h_addr_list)[0]);
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
_tcp_get_fd (stream_t stream, int *fd)
{
  struct _tcp_instance *tcp = stream_get_owner (stream);

  if (fd == NULL || tcp->fd == -1)
    return EINVAL;

  *fd = tcp->fd;
  return 0;
}

static int
_tcp_read (stream_t stream, char *buf, size_t buf_size, off_t offset, size_t * br)
{
  struct _tcp_instance *tcp = stream_get_owner (stream);
  int bytes;

  offset = offset;
  if (br == NULL)
    return EINVAL;
  *br = 0;
  if ((bytes = recv (tcp->fd, buf, buf_size, 0)) == -1)
    {
      *br = 0;
      return errno;
    }
  *br = bytes;
  return 0;
}

static int
_tcp_write (stream_t stream, const char *buf, size_t buf_size, off_t offset,
	    size_t * bw)
{
  struct _tcp_instance *tcp = stream_get_owner (stream);
  int bytes;

  offset = offset;
  if (bw == NULL)
    return EINVAL;
  *bw = 0;
  if ((bytes = send (tcp->fd, buf, buf_size, 0)) == -1)
    {
      *bw = 0;
      return errno;
    }
  *bw = bytes;
  return 0;
}

static void
_tcp_destroy (stream_t stream)
{
  struct _tcp_instance *tcp = stream_get_owner (stream);

  if (tcp->host)
    free (tcp->host);
  if (tcp->fd != -1)
    close (tcp->fd);

  if(tcp->host)
    free (tcp->host);
  free (tcp);
}

int
tcp_stream_create (stream_t * stream, const char* host, int port, int flags)
{
  struct _tcp_instance *tcp;
  int ret;

  if (host == NULL)
    return MU_ERR_TCP_NO_HOST

  if (port < 1)
    return MU_ERR_TCP_NO_PORT;

  if ((tcp = malloc (sizeof (*tcp))) == NULL)
    return ENOMEM;
  tcp->fd = -1;
  tcp->host = strdup (host);
  if(!tcp->host)
  {
    free (tcp);
    return ENOMEM;
  }
  tcp->port = port;
  tcp->state = TCP_STATE_INIT;

  if ((ret = stream_create (stream,
	  flags | MU_STREAM_NO_CHECK | MU_STREAM_RDWR, tcp)) != 0)
  {
    free (tcp->host);
    free (tcp);

    return ret;
  }

  stream_set_open (*stream, _tcp_open, tcp);
  stream_set_close (*stream, _tcp_close, tcp);
  stream_set_read (*stream, _tcp_read, tcp);
  stream_set_write (*stream, _tcp_write, tcp);
  stream_set_fd (*stream, _tcp_get_fd, tcp);
  stream_set_destroy (*stream, _tcp_destroy, tcp);

  return 0;
}
