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

static void
uint32_to_bytes (unsigned char *bytes, uint32_t u)
{
  int i;
  
  for (i = 0; i < 4; i++)
    {
      bytes[i] = u & 0xff;
      u >>= 8;
    }
}

int
_mu_inaddr_to_bytes (int af, void *buf, unsigned char *bytes)
{
  uint32_t u;
  
  switch (af)
    {
    case AF_INET:
      memcpy (&u, buf, sizeof u);
      uint32_to_bytes (bytes, u);
      return 4;

#ifdef MAILUTILS_IPV6
    case AF_INET6:
      memcpy (bytes, buf, 16);
      return 16;
#endif
    }
  return 0;
}

int
_mu_sockaddr_to_bytes (unsigned char *bytes, struct sockaddr const *sa)
{
  switch (sa->sa_family)
    {
    case AF_INET:
      uint32_to_bytes (bytes, ((struct sockaddr_in*)sa)->sin_addr.s_addr);
      return 4;

#ifdef MAILUTILS_IPV6
    case AF_INET6:
      memcpy (bytes, &((struct sockaddr_in6*)sa)->sin6_addr, 16);
      return 16;
#endif
    }
  return 0;
}

int
mu_cidr_from_sockaddr (struct mu_cidr *cidr, const struct sockaddr *sa)
{
  unsigned char address[MU_INADDR_BYTES];
  int len;
  int i;
  
  len = _mu_sockaddr_to_bytes (address, sa);
  if (len == 0)
    return MU_ERR_FAMILY;
  cidr->family = sa->sa_family;
  cidr->len = len;
  memcpy (cidr->address, address, sizeof (cidr->address));
  for (i = 0; i < MU_INADDR_BYTES; i++)
    cidr->netmask[i] = 0xff;
  return 0;
}

      
