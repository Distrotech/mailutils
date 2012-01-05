/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2011-2012 Free Software Foundation, Inc.

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

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <mailutils/sockaddr.h>
#include <mailutils/url.h>
#include <mailutils/io.h>
#include <mailutils/errno.h>
#include <mailutils/error.h>
#include <mailutils/nls.h>

static struct mu_sockaddr *
match_sa (struct mu_sockaddr *list, struct sockaddr *sa, socklen_t len)
{
  for (; list; list = list->next)
    if (len == list->addrlen && memcmp (list->addr, sa, len) == 0)
      break;
  return list;
}

int
mu_sockaddr_from_node (struct mu_sockaddr **retval, const char *node,
		       const char *serv, struct mu_sockaddr_hints *mh)
{
  int rc;

  if (!mh)
    {
      static struct mu_sockaddr_hints nullhints = { 0, AF_UNSPEC };
      mh = &nullhints;
    }

  if (mh->family == AF_UNIX)
    {
      size_t slen;
      struct sockaddr_un s_un;

      if (!node)
	return MU_ERR_NONAME;
      slen = strlen (node);
      if (slen >= sizeof s_un.sun_path)
	return MU_ERR_BUFSPACE;

      s_un.sun_family = AF_UNIX;
      strcpy(s_un.sun_path, node);
      return mu_sockaddr_create (retval, (struct sockaddr*) &s_un,
				 sizeof (s_un));
    }
  else 
#ifdef MAILUTILS_IPV6
    {
      struct addrinfo hints;
      struct addrinfo *res, *ap;
      char portbuf[64];
      struct mu_sockaddr *tail = NULL;
      
      memset (&hints, 0, sizeof (hints));
      hints.ai_family = mh->family;
      hints.ai_socktype = mh->socktype;
      hints.ai_protocol = mh->protocol;

      if (!node)
	{
	  if (mh->flags & MU_AH_PASSIVE)
	    hints.ai_flags |= AI_PASSIVE;
	  else
	    return MU_ERR_NONAME;
	}
      if (!serv && mh->port)
	{
	  snprintf (portbuf, sizeof portbuf, "%hu", mh->port);
	  serv = portbuf;
	}
	
      rc = getaddrinfo (node, serv, &hints, &res);

      switch (rc)
	{
	case 0:
	  break;

	case EAI_FAIL:
	  return MU_ERR_GETHOSTBYNAME;
	  
	case EAI_FAMILY:
	  return MU_ERR_FAMILY;

	case EAI_NONAME:
	  return MU_ERR_NONAME;
	  
	case EAI_SERVICE:
	  return MU_ERR_SERVICE;

	case EAI_SYSTEM:
	  mu_error (_("%s:%s: cannot parse address: %s"),
		    node, serv, mu_strerror (errno));
	  return errno;
			
	case EAI_BADFLAGS:
	  return MU_ERR_BADFLAGS;
	  
	case EAI_SOCKTYPE:
	  return MU_ERR_SOCKTYPE;
			
	case EAI_MEMORY:
	  return ENOMEM;
			
	default:
	  mu_error ("%s:%s: %s", node, serv, gai_strerror (rc));
	  return MU_ERR_FAILURE;
	}

      *retval = NULL;
      for (ap = res; ap; ap = ap->ai_next)
	if (mh->family == AF_UNSPEC || ap->ai_addr->sa_family == mh->family)
	  {
	    struct mu_sockaddr *sa;

	    if (match_sa (*retval, ap->ai_addr, ap->ai_addrlen))
	      continue;
	    rc = mu_sockaddr_create (&sa, ap->ai_addr, ap->ai_addrlen);
	    if (rc)
	      {
		mu_sockaddr_free_list (*retval);
		freeaddrinfo (res);
		return rc;
	      }
	    if (tail)
	      mu_sockaddr_insert (tail, sa, 0);
	    else
	      *retval = sa;
	    tail = sa;
	  }
      freeaddrinfo (res);
    }
#else
  if (mh->family == AF_INET || mh->family == AF_UNSPEC)
    {
      short port;
      struct hostent *hp;
      struct mu_sockaddr *tail = NULL;
      char **p;

      if (serv)
	{
	  char *end;
	  unsigned long n = strtoul (serv, &end, 10);

	  if (*end)
	    {
	      struct servent *sp;
	      const char *proto;

	      if (mh->protocol)
		{
		  struct protoent *pp = getprotobynumber (mh->protocol);
		  if (!pp)
		    return EINVAL;
		  proto = pp->p_name;
		}
	      else
		proto = NULL;
	      
	      sp = getservbyname (serv, proto);
	      if (!sp)
		return MU_ERR_SERVICE;
	      port = sp->s_port;
	    }
	  else if (n == 0 || (port = n) != n)
	    return MU_ERR_PARSE; /* FIXME: need MU_ERR_RANGE? */
	}
      else if (mh->port)
	port = htons (mh->port);
      else
	port = 0;
      
      if (!node)
	{
	  struct sockaddr_in s_in;

	  if (!(mh->flags & MU_AH_PASSIVE))
	    return MU_ERR_NONAME;

	  s_in.sin_family = AF_INET;
	  s_in.sin_addr.s_addr = INADDR_ANY;
	  s_in.sin_port = port;
	  return mu_sockaddr_create (retval, (struct sockaddr*)&s_in,
				     sizeof (s_in));
	}
      
      hp = gethostbyname (node);
      if (!hp)
	return MU_ERR_GETHOSTBYNAME;

      if (hp->h_addrtype != AF_INET || hp->h_length != 4)
	return MU_ERR_FAMILY;

      for (p = hp->h_addr_list; *p; p++)
	{
	  struct mu_sockaddr *sa;
	  struct sockaddr_in s_in;

	  s_in.sin_family = AF_INET;
	  memcpy(&s_in.sin_addr, *p, 4);
	  s_in.sin_port = port;
	    
	  rc = mu_sockaddr_create (&sa, (struct sockaddr*)&s_in,
				   sizeof (s_in));
	  if (rc)
	    {
	      mu_sockaddr_free_list (*retval);
	      return rc;
	    }
	  if (tail)
	    mu_sockaddr_insert (tail, sa, 0);
	  else
	    *retval = sa;
	  tail = sa;
	}
    }
  else
    return MU_ERR_FAMILY;
#endif
  return 0;
}
