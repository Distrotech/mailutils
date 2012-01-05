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
#include <errno.h>
#include <string.h>
#include <mailutils/sockaddr.h>
#include <mailutils/url.h>
#include <mailutils/io.h>
#include <mailutils/errno.h>
#include <mailutils/nls.h>
#include <mailutils/kwd.h>

static struct mu_kwd famtab[] = {
  { "unix", AF_UNIX },
  { "local", AF_UNIX },
  { "inet4", AF_INET },
#ifdef MAILUTILS_IPV6
  { "inet6", AF_INET6 },
  { "inet",  AF_UNSPEC },
#else
  { "inet",  AF_INET },
#endif
  { NULL }
};

int
mu_sockaddr_from_url (struct mu_sockaddr **retval, mu_url_t url,
		      struct mu_sockaddr_hints *mh)
{
  int rc;
  const char *scheme;
  const char *node = NULL, *serv = NULL;
  struct mu_sockaddr_hints hints;
  
  if (mh)
    memcpy (&hints, mh, sizeof (hints));
  else
    {
      memset (&hints, sizeof(hints), 0);
      hints.family = AF_UNSPEC;
      hints.socktype = SOCK_STREAM;
      hints.protocol = IPPROTO_TCP;
    }

  if (hints.family == AF_UNSPEC)
    {
      rc = mu_url_sget_scheme (url, &scheme);
      if (rc)
	return rc;

      if (mu_kwd_xlat_name (famtab, scheme, &hints.family))
	{
	  if (hints.flags & MU_AH_DETECT_FAMILY)
	    {
	      int flags = 0;
	      
	      mu_url_get_flags (url, &flags);
#ifdef MAILUTILS_IPV6
	      if (flags & MU_URL_IPV6)
		hints.family = AF_INET6;
	      else
#endif
		if (flags & (MU_URL_HOST|MU_URL_PORT))
		  hints.family = AF_INET;
	      else if (flags & MU_URL_PATH)
		hints.family = AF_UNIX;
	      else
		return MU_ERR_FAMILY;
	    }
	  else
	    return MU_ERR_FAMILY;
	}
    }
  
  if (hints.family == AF_UNIX)
    {
      rc = mu_url_sget_path (url, &node);
      if (rc)
	{
	  if (rc == MU_ERR_NOENT)
	    {
	      rc = mu_url_sget_host (url, &node);
	      if (rc == MU_ERR_NOENT)
		return MU_ERR_NONAME;
	    }
	  if (rc)
	    return rc;
	}
    }
  else
    {
#ifdef MAILUTILS_IPV6
      if (hints.family == AF_UNSPEC)
	hints.family = mu_url_has_flag (url, MU_URL_IPV6) ? AF_INET6 : AF_INET;
#endif
      rc = mu_url_sget_host (url, &node);
      if (rc && rc != MU_ERR_NOENT)
	return MU_ERR_NONAME;
      rc = mu_url_sget_portstr (url, &serv);
      if (rc && rc != MU_ERR_NOENT)
	return MU_ERR_NONAME;
    }
  return mu_sockaddr_from_node (retval, node, serv, &hints);
}
