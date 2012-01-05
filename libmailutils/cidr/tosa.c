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
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <mailutils/cidr.h>
#include <mailutils/errno.h>

int
mu_cidr_to_sockaddr (struct mu_cidr *cidr, struct sockaddr **psa)
{
  union
  {
    struct sockaddr sa;
    struct sockaddr_in s_in;
#ifdef MAILUTILS_IPV6
    struct sockaddr_in6 s_in6;
#endif
  } addr;
  struct sockaddr *sa;
  int socklen;
  int i;
  
  memset (&addr, 0, sizeof (addr));
  addr.sa.sa_family = cidr->family;
  switch (cidr->family)
    {
    case AF_INET:
      socklen = sizeof (addr.s_in);
      for (i = 0; i < cidr->len; i++)
	{
	  addr.s_in.sin_addr.s_addr <<= 8;
	  addr.s_in.sin_addr.s_addr |= cidr->address[i];
	}
      break;

#ifdef MAILUTILS_IPV6
    case AF_INET6:
      socklen = sizeof (addr.s_in6);
      memcpy (&addr.s_in6.sin6_addr, cidr->address, 16);
      break;
#endif

    default:
      return MU_ERR_FAMILY;
    }

  sa = malloc (socklen);
  if (!sa)
    return ENOMEM;
  memcpy (sa, &addr, socklen);
  *psa = sa;
  return 0;
}

