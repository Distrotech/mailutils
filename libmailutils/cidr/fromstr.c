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
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <mailutils/cidr.h>
#include <mailutils/errno.h>
#include <mailutils/sockaddr.h>

static void
masklen_to_netmask (unsigned char *buf, size_t len, size_t masklen)
{
  int i, cnt;

  cnt = masklen / 8;
  for (i = 0; i < cnt; i++)
    buf[i] = 0xff;
  if (i == MU_INADDR_BYTES)
    return;
  cnt = 8 - masklen % 8;
  buf[i++] = (0xff >> cnt) << cnt;
  for (; i < MU_INADDR_BYTES; i++)
    buf[i] = 0;
}

int
mu_cidr_from_string (struct mu_cidr *pcidr, const char *str)
{
  int rc;
  char ipbuf[41];
  struct mu_cidr cidr;
  char *p;
  size_t len;
  union
  {
    struct in_addr in;
#ifdef MAILUTILS_IPV6
    struct in6_addr in6;
#endif
  } inaddr;
  
  p = strchr (str, '/');
  if (p)
    len = p - str;
  else
    len = strlen (str);

  if (len > sizeof (ipbuf))
    return MU_ERR_BUFSPACE;
  
  memcpy (ipbuf, str, len);
  ipbuf[len] = 0;

  if (mu_str_is_ipv4 (ipbuf))
    cidr.family = AF_INET;
#ifdef MAILUTILS_IPV6
  else if (mu_str_is_ipv6 (ipbuf))
    cidr.family = AF_INET6;
#endif
  else
    return MU_ERR_FAMILY;

  rc = inet_pton (cidr.family, ipbuf, &inaddr);
  if (rc == -1)
    return MU_ERR_FAMILY;
  else if (rc == 0)
    return MU_ERR_NONAME;
  else if (rc != 1)
    return MU_ERR_FAILURE;

  cidr.len = _mu_inaddr_to_bytes (cidr.family, &inaddr, cidr.address);
  if (cidr.len == 0)
    return MU_ERR_FAMILY;

  if (p)
    {
      char *end;
      unsigned long masklen;
      
      p++;

      masklen = strtoul (p, &end, 10);
      if (*end == 0)
	masklen_to_netmask (cidr.netmask, cidr.len, masklen);
      else if ((cidr.family == AF_INET && mu_str_is_ipv4 (p))
#ifdef MAILUTILS_IPV6
	       || (cidr.family == AF_INET6 && mu_str_is_ipv6 (ipbuf))
#endif
	       )
	{
	  rc = inet_pton (cidr.family, p, &inaddr);
	  if (rc == -1)
	    return MU_ERR_FAMILY;
	  else if (rc == 0)
	    return MU_ERR_NONAME;
	  else if (rc != 1)
	    return MU_ERR_FAILURE;

	  _mu_inaddr_to_bytes (cidr.family, &inaddr, cidr.netmask);
	}
      else
	return MU_ERR_FAMILY;
    }
  else
    masklen_to_netmask (cidr.netmask, cidr.len, cidr.len * 8);

  memcpy (pcidr, &cidr, sizeof (*pcidr));
  return 0;
}

