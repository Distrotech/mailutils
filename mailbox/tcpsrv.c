/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2007 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library; If not, see
   <http://www.gnu.org/licenses/>.  */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <mailutils/acl.h>
#include <mailutils/server.h>
#include <mailutils/debug.h>
#include <mailutils/diag.h>
#include <mailutils/errno.h>
#include <mailutils/nls.h>


struct _mu_tcp_server
{
  char *ident;
  struct sockaddr *addr;
  int addrlen;
  int backlog;
  int fd; 
  mu_debug_t debug;
  mu_acl_t acl;
  mu_tcp_server_conn_fp f_conn;
  mu_tcp_server_intr_fp f_intr;
  mu_tcp_server_free_fp f_free;
  void *data;
};

#define IDENTSTR(s) ((s)->ident ? (s)->ident : "default")

int
mu_tcp_server_create (mu_tcp_server_t *psrv, struct sockaddr *addr,
		      int addrlen)
{
  struct _mu_tcp_server *srv;
  mu_log_level_t level;
  
  srv = calloc (1, sizeof *srv);
  if (!srv)
    return ENOMEM;
  srv->addr = calloc (1, addrlen);
  if (!srv->addr)
    {
      free (srv);
      return ENOMEM;
    }
  memcpy (srv->addr, addr, addrlen);
  srv->addrlen = addrlen;
  level = mu_global_debug_level ("tcp_server");
  if (level)
    {
      mu_debug_create (&srv->debug, NULL);
      mu_debug_set_level (srv->debug, level);
    }
  srv->fd = -1;
  srv->backlog = 4;
  *psrv = srv;
  return 0;
}

int
mu_tcp_server_destroy (mu_tcp_server_t *psrv)
{
  mu_tcp_server_t srv;
  if (!psrv)
    return EINVAL;
  srv = *psrv;
  if (!srv)
    return 0;
  if (srv->f_free)
    srv->f_free (srv->data);
  close (srv->fd);
  free (srv->addr);
  free (srv->ident);
  free (srv);
  *psrv = NULL;
  return 0;
}

int
mu_tcp_server_set_debug (mu_tcp_server_t srv, mu_debug_t debug)
{
  if (!srv)
    return EINVAL;
  mu_debug_destroy (&srv->debug, NULL);
  srv->debug = debug;
  return 0;
}

int
mu_tcp_server_get_debug (mu_tcp_server_t srv, mu_debug_t *pdebug)
{
  if (!srv)
    return EINVAL;
  *pdebug = srv->debug;
  return 0;
}

int
mu_tcp_server_set_backlog (mu_tcp_server_t srv, int backlog)
{
  if (!srv)
    return EINVAL;
  srv->backlog = backlog;
  return 0;
}

int
mu_tcp_server_set_ident (mu_tcp_server_t srv, const char *ident)
{
  if (!srv)
    return EINVAL;
  if (srv->ident)
    free (srv->ident);
  srv->ident = strdup (ident);
  if (!srv->ident)
    return ENOMEM;
  return 0;
}

int
mu_tcp_server_set_acl (mu_tcp_server_t srv, mu_acl_t acl)
{
  if (!srv)
    return EINVAL;
  srv->acl = acl;
  return 0;
}

int
mu_tcp_server_set_conn (mu_tcp_server_t srv, mu_tcp_server_conn_fp conn)
{
  if (!srv)
    return EINVAL;
  srv->f_conn = conn;
  return 0;
}

int
mu_tcp_server_set_intr (mu_tcp_server_t srv, mu_tcp_server_intr_fp intr)
{
  if (!srv)
    return EINVAL;
  srv->f_intr = intr;
  return 0;
}

int
mu_tcp_server_set_data (mu_tcp_server_t srv,
			void *data, mu_tcp_server_free_fp free)
{
  if (!srv)
    return EINVAL;
  srv->data = data;
  srv->f_free = free;
  return 0;
}

static int
family_to_proto (int family)
{
  switch (family)
    {
    case AF_UNIX:
      return PF_UNIX;

    case AF_INET:
      return PF_INET;

    default:
      abort ();
    }
}

int
mu_tcp_server_open (mu_tcp_server_t srv)
{
  int fd;
  
  if (!srv || srv->fd != -1)
    return EINVAL;

  if (mu_debug_check_level (srv->debug, MU_DEBUG_TRACE0))
    {
      char *p = mu_sockaddr_to_astr (srv->addr, srv->addrlen);
      __MU_DEBUG2 (srv->debug, MU_DEBUG_TRACE0,
		   "opening server \"%s\" %s\n", IDENTSTR (srv),
		   p);
      free (p);
    }

  fd = socket (family_to_proto (srv->addr->sa_family), SOCK_STREAM, 0);
  if (fd == -1)
    {
      MU_DEBUG2 (srv->debug, MU_DEBUG_ERROR,
		 "%s: socket: %s\n", IDENTSTR (srv), mu_strerror (errno));
      return errno;
    }
  
  switch (srv->addr->sa_family)
    {
    case AF_UNIX:
      {
	struct stat st;
	struct sockaddr_un *s_un = (struct sockaddr_un *) srv->addr;
	
	if (stat (s_un->sun_path, &st))
	  {
	    if (errno != ENOENT)
	      {
		MU_DEBUG3 (srv->debug, MU_DEBUG_ERROR,
			   _("%s: file %s exists but cannot be stat'd: %s"),
			   IDENTSTR (srv),
			   s_un->sun_path,
			   mu_strerror (errno));
		return EAGAIN;
	      }
	  }
	else if (!S_ISSOCK (st.st_mode))
	  {
	    MU_DEBUG2 (srv->debug, MU_DEBUG_ERROR,
		       _("%s: file %s is not a socket"),
		       IDENTSTR (srv), s_un->sun_path);
	    return EAGAIN;
	  }
	else if (unlink (s_un->sun_path))
	  {
	    MU_DEBUG3 (srv->debug, MU_DEBUG_ERROR,
		       _("%s: cannot unlink file %s: %s"),
		       IDENTSTR (srv), s_un->sun_path, mu_strerror (errno));
	    return EAGAIN;
	  }
      }
      break;

    case AF_INET:
      {
	int t;
	
	t = 1;	 
	setsockopt (fd, SOL_SOCKET, SO_REUSEADDR, &t, sizeof (t));
      }
    }
  
  if (bind (fd, srv->addr, srv->addrlen) == -1)
    {
      MU_DEBUG2 (srv->debug, MU_DEBUG_ERROR,
		 "%s: bind: %s\n", IDENTSTR (srv), mu_strerror (errno));
      close (fd);
      return errno;
    }

  if (listen (fd, srv->backlog) == -1) 
    {
      MU_DEBUG2 (srv->debug, MU_DEBUG_ERROR,
		 "%s: listen: %s\n", IDENTSTR (srv), mu_strerror (errno));
      close (fd);
      return errno;
    }
  
  srv->fd = fd;
  return 0;
}

int
mu_tcp_server_shutdown (mu_tcp_server_t srv)
{
  if (!srv || srv->fd != -1)
    return EINVAL;
  if (mu_debug_check_level (srv->debug, MU_DEBUG_TRACE0))
    {
      char *p = mu_sockaddr_to_astr (srv->addr, srv->addrlen);
      __MU_DEBUG2 (srv->debug, MU_DEBUG_TRACE0,
		   "closing server \"%s\" %s\n", IDENTSTR (srv),
		   p);
      free (p);
    }
  close (srv->fd);
  return 0;
}

int
mu_tcp_server_accept (mu_tcp_server_t srv, void *call_data)
{
  int rc;
  int connfd;
  union
  {
    struct sockaddr sa;
    char buffer[512];
  } client;
  
  socklen_t size = sizeof (client);
  
  if (!srv || srv->fd == -1)
    return EINVAL;

  connfd = accept (srv->fd, &client.sa, &size);
  if (connfd == -1)
    {
      int ec = errno;
      if (ec == EINTR)
	{
	  if (srv->f_intr && srv->f_intr (srv->data, call_data))
	    mu_tcp_server_shutdown (srv);
	}
      return ec;
    }

  if (srv->acl)
    {
      mu_acl_result_t res;
      int rc = mu_acl_check_sockaddr (srv->acl, &client.sa, size, &res);
      if (rc)
	MU_DEBUG2 (srv->debug, MU_DEBUG_ERROR,
		   "%s: mu_acl_check_sockaddr: %s\n",
		   IDENTSTR (srv), strerror (rc));
      if (res == mu_acl_result_deny)
	{
	  char *p = mu_sockaddr_to_astr (srv->addr, srv->addrlen);
	  mu_diag_output (MU_DIAG_INFO, "Denying connection from %s", p);
	  free (p);
	  
	  close (connfd);
	  return 0;
	}
    }
  rc = srv->f_conn (connfd, &client.sa, size, srv->data, call_data, srv);
  if (rc)
    mu_tcp_server_shutdown (srv);
  close (connfd);
  return rc;
}

int
mu_tcp_server_loop (mu_tcp_server_t srv, void *call_data)
{
  if (!srv)
    return EINVAL;
  while (srv->fd != -1)
    {
      int rc = mu_tcp_server_accept (srv, call_data);
      if (rc && rc != EINTR)
	{
	  mu_tcp_server_shutdown (srv);
	  return rc;
	}
    }
  return 0;
}

int
mu_tcp_server_get_fd (mu_tcp_server_t srv)
{
  return srv->fd;
}

int
mu_tcp_server_get_sockaddr (mu_tcp_server_t srv, struct sockaddr *s, int *size)
{
  int len;
  
  if (!srv || !s)
    return EINVAL;
  if (s == 0)
    len = srv->addrlen;
  else
    {
      len = *size;
      if (len < srv->addrlen)
	len = srv->addrlen;
      memcpy (s, srv->addr, len);
    }
  *size = len;
  return 0;
}
  
