/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2007-2008, 2010-2012 Free Software Foundation, Inc.

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
#include <mailutils/sockaddr.h>


struct _mu_ip_server
{
  char *ident;
  struct mu_sockaddr *addr;
  int fd;
  int type;
  mu_acl_t acl;
  mu_ip_server_conn_fp f_conn;
  mu_ip_server_intr_fp f_intr;
  mu_ip_server_free_fp f_free;
  void *data;
  union
  {
    struct
    {
      int backlog;
    } tcp_data;
    struct
    {
      char *buf;
      size_t bufsize;
      ssize_t rdsize;
    } udp_data;
  } v;
};

#define IDENTSTR(s) ((s)->ident ? (s)->ident : "default")

int
mu_ip_server_create (mu_ip_server_t *psrv, struct mu_sockaddr *addr, int type)
{
  struct _mu_ip_server *srv;

  switch (type)
    {
    case MU_IP_UDP:
    case MU_IP_TCP:
      break;
      
    default:
      return EINVAL;
    }
  
  srv = calloc (1, sizeof *srv);
  if (!srv)
    return ENOMEM;
  srv->addr = addr;
  srv->type = type;
  srv->fd = -1;
  switch (type)
    {
    case MU_IP_UDP:
      srv->v.udp_data.bufsize = 4096;
      break;
      
    case MU_IP_TCP:
      srv->v.tcp_data.backlog = 4;
    }
      
  *psrv = srv;
  return 0;
}

int
mu_ip_server_destroy (mu_ip_server_t *psrv)
{
  mu_ip_server_t srv;
  if (!psrv)
    return EINVAL;
  srv = *psrv;
  if (!srv)
    return 0;
  if (srv->f_free)
    srv->f_free (srv->data);
  close (srv->fd);
  mu_sockaddr_free (srv->addr);
  free (srv->ident);
  if (srv->type == MU_IP_UDP && srv->v.udp_data.buf)
    free (srv->v.udp_data.buf);
  free (srv);
  *psrv = NULL;
  return 0;
}

int
mu_ip_server_get_type (mu_ip_server_t srv, int *ptype)
{
  if (!srv)
    return EINVAL;
  *ptype = srv->type;
  return 0;
}

int
mu_tcp_server_set_backlog (mu_ip_server_t srv, int backlog)
{
  if (!srv || srv->type != MU_IP_TCP)
    return EINVAL;
  srv->v.tcp_data.backlog = backlog;
  return 0;
}

int
mu_udp_server_get_bufsize (mu_ip_server_t srv, size_t *psize)
{
  if (!srv || srv->type != MU_IP_UDP)
    return EINVAL;
  *psize = srv->v.udp_data.bufsize;
  return 0;
}

int
mu_udp_server_set_bufsize (mu_ip_server_t srv, size_t size)
{
  if (!srv || srv->type != MU_IP_UDP)
    return EINVAL;
  srv->v.udp_data.bufsize = size;
  if (srv->v.udp_data.buf)
    {
      char *p = realloc (srv->v.udp_data.buf, size);
      if (!p)
	return ENOMEM;
      srv->v.udp_data.buf = p;
    }
  return 0;
}  

int
mu_ip_server_set_ident (mu_ip_server_t srv, const char *ident)
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
mu_ip_server_set_acl (mu_ip_server_t srv, mu_acl_t acl)
{
  if (!srv)
    return EINVAL;
  srv->acl = acl;
  return 0;
}

int
mu_ip_server_set_conn (mu_ip_server_t srv, mu_ip_server_conn_fp conn)
{
  if (!srv)
    return EINVAL;
  srv->f_conn = conn;
  return 0;
}

int
mu_ip_server_set_intr (mu_ip_server_t srv, mu_ip_server_intr_fp intr)
{
  if (!srv)
    return EINVAL;
  srv->f_intr = intr;
  return 0;
}

int
mu_ip_server_set_data (mu_ip_server_t srv,
			void *data, mu_ip_server_free_fp free)
{
  if (!srv)
    return EINVAL;
  srv->data = data;
  srv->f_free = free;
  return 0;
}

int
mu_address_family_to_domain (int family)
{
  switch (family)
    {
    case AF_UNIX:
      return PF_UNIX;

    case AF_INET:
      return PF_INET;

#ifdef MAILUTILS_IPV6
    case AF_INET6:
      return PF_INET6;
#endif
      
    default:
      abort ();
    }
}

int
mu_ip_server_open (mu_ip_server_t srv)
{
  int fd;
  
  if (!srv || srv->fd != -1)
    return EINVAL;

  mu_debug (MU_DEBCAT_SERVER, MU_DEBUG_TRACE0,
	    ("opening server \"%s\" %s", IDENTSTR (srv),
	     mu_sockaddr_str (srv->addr)));

  fd = socket (mu_address_family_to_domain (srv->addr->addr->sa_family),
	       ((srv->type == MU_IP_UDP) ? SOCK_DGRAM : SOCK_STREAM), 0);
  if (fd == -1)
    {
      mu_debug (MU_DEBCAT_SERVER, MU_DEBUG_ERROR,
		("%s: socket: %s", IDENTSTR (srv), mu_strerror (errno)));
      return errno;
    }
  
  switch (srv->addr->addr->sa_family)
    {
    case AF_UNIX:
      {
	struct stat st;
	struct sockaddr_un *s_un = (struct sockaddr_un *) srv->addr;
	
	if (stat (s_un->sun_path, &st))
	  {
	    if (errno != ENOENT)
	      {
		mu_debug (MU_DEBCAT_SERVER, MU_DEBUG_ERROR,
			  ("%s: file %s exists but cannot be stat'd: %s",
			   IDENTSTR (srv),
			   s_un->sun_path,
			   mu_strerror (errno)));
		return EAGAIN;
	      }
	  }
	else if (!S_ISSOCK (st.st_mode))
	  {
	    mu_debug (MU_DEBCAT_SERVER, MU_DEBUG_ERROR,
		      ("%s: file %s is not a socket",
		       IDENTSTR (srv), s_un->sun_path));
	    return EAGAIN;
	  }
	else if (unlink (s_un->sun_path))
	  {
            mu_debug (MU_DEBCAT_SERVER,  MU_DEBUG_ERROR,
		      ("%s: cannot unlink file %s: %s",
		       IDENTSTR (srv), s_un->sun_path, mu_strerror (errno)));
	    return EAGAIN;
	  }
      }
      break;

    default:
      {
	int t;
	
	t = 1;	 
	setsockopt (fd, SOL_SOCKET, SO_REUSEADDR, &t, sizeof (t));
      }
    }
  
  if (bind (fd, srv->addr->addr, srv->addr->addrlen) == -1)
    {
      mu_debug (MU_DEBCAT_SERVER, MU_DEBUG_ERROR,
		("%s: bind: %s", IDENTSTR (srv), mu_strerror (errno)));
      close (fd);
      return errno;
    }

  if (srv->type == MU_IP_TCP)
    {
      if (listen (fd, srv->v.tcp_data.backlog) == -1) 
	{
	  mu_debug (MU_DEBCAT_SERVER, MU_DEBUG_ERROR,
		    ("%s: listen: %s", IDENTSTR (srv), mu_strerror (errno)));
	  close (fd);
	  return errno;
	}
    }
  
  srv->fd = fd;
  return 0;
}

int
mu_ip_server_shutdown (mu_ip_server_t srv)
{
  if (!srv || srv->fd != -1)
    return EINVAL;
  mu_debug (MU_DEBCAT_SERVER, MU_DEBUG_TRACE0,
	    ("closing server \"%s\" %s", IDENTSTR (srv),
	     mu_sockaddr_str (srv->addr)));
  close (srv->fd);
  return 0;
}

int
mu_ip_tcp_accept (mu_ip_server_t srv, void *call_data)
{
  int rc;
  int connfd;
  union
  {
    struct sockaddr sa;
    struct sockaddr_in s_in;
    struct sockaddr_un s_un;
#ifdef MAILUTILS_IPV6
    struct sockaddr_in6 s_in6;
#endif
  } client;
  
  socklen_t size = sizeof (client);
  
  if (!srv || srv->fd == -1 || srv->type == MU_IP_UDP)
    return EINVAL;

  connfd = accept (srv->fd, &client.sa, &size);
  if (connfd == -1)
    {
      int ec = errno;
      if (ec == EINTR)
	{
	  if (srv->f_intr && srv->f_intr (srv->data, call_data))
	    mu_ip_server_shutdown (srv);
	}
      return ec;
    }

  if (srv->acl)
    {
      mu_acl_result_t res;
      int rc;
      
      mu_acl_set_session_id (srv->acl);
      rc = mu_acl_check_sockaddr (srv->acl, &client.sa, size, &res);
      if (rc)
	mu_debug (MU_DEBCAT_SERVER, MU_DEBUG_ERROR,
		  ("%s: mu_acl_check_sockaddr: %s",
		   IDENTSTR (srv), strerror (rc)));
      if (res == mu_acl_result_deny)
	{
	  char *p = mu_sockaddr_to_astr (&client.sa, size);
	  mu_diag_output (MU_DIAG_INFO, "Denying connection from %s", p);
	  free (p);
	  
	  close (connfd);
	  return 0;
	}
    }
  rc = srv->f_conn (connfd, &client.sa, size, srv->data, call_data, srv);
  close (connfd);
  return rc;
}

int
mu_ip_udp_accept (mu_ip_server_t srv, void *call_data)
{
  int rc;
  union
  {
    struct sockaddr sa;
    struct sockaddr_in s_in;
    struct sockaddr_un s_un;
#ifdef MAILUTILS_IPV6
    struct sockaddr_in6 s_in6;
#endif
  } client;
  fd_set rdset;
  
  socklen_t salen = sizeof (client);
  ssize_t size;

  if (!srv->v.udp_data.buf)
    {
      srv->v.udp_data.buf = malloc (srv->v.udp_data.bufsize);
      if (!srv->v.udp_data.buf)
	return ENOMEM;
    }
  
  FD_ZERO (&rdset);
  FD_SET (srv->fd, &rdset);
  for (;;)
    {
      rc = select (srv->fd + 1, &rdset, NULL, NULL, NULL);
      if (rc == -1)
	{
	  if (errno == EINTR)
	    {
	      if (srv->f_intr && srv->f_intr (srv->data, call_data))
		break;
	      else
		continue;
	    }
	}
      else
	break;
    }

  if (rc == -1)
    return errno;

  size = recvfrom (srv->fd, srv->v.udp_data.buf, srv->v.udp_data.bufsize,
		   0, &client.sa, &salen);
  if (size < 0)
    {
      mu_debug (MU_DEBCAT_SERVER, MU_DEBUG_ERROR,
		("%s: recvfrom: %s",
		 IDENTSTR (srv), strerror (errno)));
      return MU_ERR_FAILURE;
    }
  srv->v.udp_data.rdsize = size;
  
  if (srv->acl)
    {
      mu_acl_result_t res;
      int rc;

      mu_acl_set_session_id (srv->acl);
      rc = mu_acl_check_sockaddr (srv->acl, &client.sa, size, &res);
      if (rc)
	mu_debug (MU_DEBCAT_SERVER, MU_DEBUG_ERROR,
		  ("%s: mu_acl_check_sockaddr: %s\n",
		   IDENTSTR (srv), strerror (rc)));
      if (res == mu_acl_result_deny)
	{
	  char *p = mu_sockaddr_to_astr (&client.sa, salen);
	  mu_diag_output (MU_DIAG_INFO, "Denying connection from %s", p);
	  free (p);
	  return 0;
	}
    }
  rc = srv->f_conn (-1, &client.sa, size, srv->data, call_data, srv);
  return rc;
}

int
mu_ip_server_accept (mu_ip_server_t srv, void *call_data)
{
  int rc;
  if (!srv || srv->fd == -1)
    return EINVAL;
  switch (srv->type)
    {
    case MU_IP_UDP:
      rc = mu_ip_udp_accept (srv, call_data);
      break;

    case MU_IP_TCP:
      rc = mu_ip_tcp_accept (srv, call_data);
    }
  
  if (rc)
    mu_ip_server_shutdown (srv);
  return rc;
}

int
mu_ip_server_loop (mu_ip_server_t srv, void *call_data)
{
  if (!srv)
    return EINVAL;
  while (srv->fd != -1)
    {
      int rc = mu_ip_server_accept (srv, call_data);
      if (rc && rc != EINTR)
	{
	  mu_ip_server_shutdown (srv);
	  return rc;
	}
    }
  return 0;
}

int
mu_ip_server_get_fd (mu_ip_server_t srv)
{
  return srv->fd;
}

int
mu_udp_server_get_rdata (mu_ip_server_t srv, char **pbuf, size_t *pbufsize)
{
  if (!srv || srv->type != MU_IP_UDP)
    return EINVAL;
  *pbuf = srv->v.udp_data.buf;
  *pbufsize = srv->v.udp_data.rdsize;
  return 0;
}

int
mu_ip_server_get_sockaddr (mu_ip_server_t srv, struct mu_sockaddr **psa)
{
  if (!srv || !psa)
    return EINVAL;
  return mu_sockaddr_copy (psa, srv->addr);
}

const char *
mu_ip_server_addrstr (mu_ip_server_t srv)
{
  return mu_sockaddr_str (srv->addr);
}

