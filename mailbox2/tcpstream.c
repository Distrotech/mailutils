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
_stream_tcp_cleanup (void *arg)
{
  struct _stream_tcp *tcp = arg;
  mu_refcount_unlock (tcp->base.refcount);
}

void
_stream_tcp_destroy (stream_t *pstream)
{
  struct _stream_tcp *tcp = (struct _stream_tcp *)*pstream;
  if (mu_refcount_dec (tcp->base.refcount) == 0)
    {
      _stream_fd_dtor (*pstream);
      if (tcp->host)
	free (tcp->host);
      free (tcp);
    }
}

static int
_stream_tcp_close0 (stream_t stream)
{
  struct _stream_tcp *tcp = (struct _stream_tcp *)stream;

  tcp->base.state = MU_STREAM_STATE_CLOSE;
  if (tcp->base.fd >= 0)
    close (tcp->base.fd);
  if (tcp->host)
    free (tcp->host);
  tcp->host = NULL;
  tcp->state = TCP_STATE_INIT;
  return 0;
}

int
_stream_tcp_close (stream_t stream)
{
  struct _stream_tcp *tcp = (struct _stream_tcp *)stream;

  _stream_fd_close (stream);
  if (tcp->host)
    free (tcp->host);
  tcp->host = NULL;
  tcp->state = TCP_STATE_INIT;
  return 0;
}

static int
_stream_tcp_open0 (stream_t stream, const char *host, int port, int flags)
{
  struct _stream_tcp 	*tcp = (struct _stream_tcp *)stream;
  int flgs, ret;
  size_t namelen;
  struct sockaddr_in peer_addr;
  struct hostent *phe;
  struct sockaddr_in soc_addr;

  tcp->base.state = MU_STREAM_STATE_OPEN;
  if (tcp->state == TCP_STATE_INIT)
    {
      tcp->port = port;
      if (tcp->host)
	free (tcp->host);
      tcp->host = strdup (host);
      if (tcp->host == NULL)
	return MU_ERROR_NO_MEMORY;
      tcp->base.flags = flags;
    }

  switch (tcp->state)
    {
    case TCP_STATE_INIT:
      if (tcp->base.fd == -1)
	{
	  tcp->base.fd = socket (AF_INET, SOCK_STREAM, 0);
	  if (tcp->base.fd == -1)
	    return errno;
	}
      if (tcp->base.flags & MU_STREAM_NONBLOCK)
	{
	  flgs = fcntl (tcp->base.fd, F_GETFL);
	  flgs |= O_NONBLOCK;
	  fcntl (tcp->base.fd, F_SETFL, flgs);
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
	      _stream_tcp_close0 (stream);
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

      if ((connect (tcp->base.fd, (struct sockaddr *)&soc_addr,
		    sizeof soc_addr)) == -1)
	{
	  ret = errno;
	  if (ret == EINPROGRESS || ret == EAGAIN)
	    {
	      tcp->state = TCP_STATE_CONNECTING;
	      ret = MU_ERROR_TRY_AGAIN;
	    }
	  else
	    _stream_tcp_close0 (stream);
	  return ret;
	}
      tcp->state = TCP_STATE_CONNECTING;

    case TCP_STATE_CONNECTING:
      namelen = sizeof peer_addr;
      if (getpeername (tcp->base.fd, (struct sockaddr *)&peer_addr,
		       &namelen) == 0)
	tcp->state = TCP_STATE_CONNECTED;
      else
	{
	  ret = errno;
	  _stream_tcp_close0 (stream);
	  return ret;
	}
      break;
    }
  return 0;
}

int
_stream_tcp_open (stream_t stream, const char *host, int port, int flags)
{
  int status;
  struct _stream_tcp *tcp = (struct _stream_tcp *)stream;
  mu_refcount_lock (tcp->base.refcount);
  monitor_cleanup_push (_stream_tcp_cleanup, tcp);
  status = _stream_tcp_open0 (stream, host, port, flags);
  mu_refcount_unlock (tcp->base.refcount);
  monitor_cleanup_pop (0);
  return status;
}

static struct _stream_vtable _stream_tcp_vtable;

int
stream_tcp_create (stream_t *pstream)
{
  struct _stream_tcp *tcp;
  stream_t stream;
  int status;

  if (pstream == NULL)
    return MU_ERROR_INVALID_PARAMETER;

  tcp = calloc (1, sizeof *tcp);
  if (tcp == NULL)
    {
      stream_destroy (&stream);
      return MU_ERROR_NO_MEMORY;
    }

  /* Create the base.  */
  status = _stream_fd_ctor (&tcp->base, -1);
  if (status != 0)
    return status;

  tcp->host = NULL;
  tcp->port = -1;
  tcp->state = TCP_STATE_INIT;
  _stream_tcp_vtable = *(tcp->base.base.vtable);
  /* Overload.  */
  _stream_tcp_vtable.open = _stream_tcp_open;
  _stream_tcp_vtable.close = _stream_tcp_close;
  _stream_tcp_vtable.destroy = _stream_tcp_destroy;

  tcp->base.base.vtable = &_stream_tcp_vtable;
  *pstream = &tcp->base.base;
  return 0;
}
