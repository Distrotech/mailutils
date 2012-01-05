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
#include <string.h>
#include <stdlib.h>
#include <mailutils/cidr.h>
#include <mailutils/errno.h>

static int
to_xdig (unsigned char b)
{
  if (b >= 0xa)
    return 'A' + b - 0xa;
  else
    return '0' + b;
}

static size_t
format_ipv6_bytes (const unsigned char *bytes, int len, 
		   char *buf, size_t size, int simplify)
{
  size_t total = 0;
  int i;
  int run_count = 0;
  char *p;
  
  for (i = 0; i < len; i += 2)
    {
      if (bytes[0] == 0 && bytes[1] == 0)
	{
	  if (simplify)
	    run_count++;
	  else
	    {
	      if (i && total++ < size)
		*buf++ = ':';
	      if (total++ < size)
		*buf++ = '0';
	    }
	  bytes += 2;
	}
      else
	{
	  if (run_count)
	    {
	      if (run_count == 1)
		{
		  if (i && total++ < size)
		    *buf++ = ':';
		  if (total++ < size)
		    *buf++ = '0';
		}
	      else
		{
		  if (total++ < size)
		    *buf++ = ':';
		  simplify = 0;
		}
	      run_count = 0;
	    }
	  
	  if (i && total++ < size)
	    *buf++ = ':';

	  p = buf;
	  if ((*bytes & 0xf0) && total++ < size)
	    *buf++ = to_xdig (*bytes >> 4);
	  if ((buf > p || (*bytes & 0xf)) && total++ < size)
	    *buf++ = to_xdig (*bytes & 0xf);
	  bytes++;
	  if ((buf > p || (*bytes & 0xf0)) && total++ < size)
	    *buf++ = to_xdig (*bytes >> 4);
	  if ((buf > p || (*bytes & 0xf)) && total++ < size)
	    *buf++ = to_xdig (*bytes & 0xf);
	  bytes++;
	}
    }

  if (run_count)
    {
      if (run_count == 1)
	{
	  if (i && total++ < size)
	    *buf++ = ':';
	  if (total++ < size)
	    *buf++ = '0';
	}
      else
	{
	  if (total++ < size)
	    *buf++ = ':';
	  if (total++ < size)
	    *buf++ = ':';
	}
    }
  
  return total;
}

static size_t
format_ipv6_bytes_normal (const unsigned char *bytes, int len, 
			  char *buf, size_t size)
{
  return format_ipv6_bytes (bytes, len, buf, size, 0);
}

static size_t
format_ipv6_bytes_simplified (const unsigned char *bytes, int len, 
			      char *buf, size_t size)
{
  return format_ipv6_bytes (bytes, len, buf, size, 1);
}


static size_t
format_ipv4_bytes (const unsigned char *bytes, int len, 
		   char *buf, size_t size)
{
  int i;
  size_t total = 0;
  
  for (i = 0; i < len; i++)
    {
      unsigned char b = *bytes++;
      char nbuf[3];
      int j;
      
      if (i)
	{
	  if (total++ < size)
	    *buf++ = '.';
	}

      j = 0;
      do
	{
	  nbuf[j++] = b % 10 + '0';
	  b /= 10;
	}
      while (b);

      for (; j; j--)
	{
	  if (total++ < size)
	    *buf++ = nbuf[j - 1];
	}
    }
  return total;
}

int
mu_cidr_to_string (struct mu_cidr *cidr, int flags,
		   char *buf, size_t size, size_t *pret)
{
  size_t (*fmt) (const unsigned char *bytes, int len, char *buf, size_t size);
  size_t n, total = 0;
  
  if (size == 0)
    return MU_ERR_BUFSPACE;
  size--;
  switch (cidr->family)
    {
    case AF_INET:
      fmt = format_ipv4_bytes;
      break;
      
#ifdef MAILUTILS_IPV6
    case AF_INET6:
      fmt = (flags & MU_CIDR_FMT_SIMPLIFY) ?
	         format_ipv6_bytes_simplified : format_ipv6_bytes_normal;
      break;
#endif

    default:
      return MU_ERR_FAMILY;
    }

  n = fmt (cidr->address, cidr->len, buf, size);
  if (buf)
    buf += n;
  total += n;

  if (!(flags & MU_CIDR_FMT_ADDRONLY))
    {
      if (total++ < size)
	*buf++ = '/';
      n = fmt (cidr->netmask, cidr->len, buf, size - total);
      if (buf)
	buf += n;
      total += n;
    }
  
  if (buf)
    *buf++ = 0;
  if (pret)
    *pret = total;
  return 0;
}

int
mu_cidr_format (struct mu_cidr *cidr, int flags, char **pbuf)
{
  char buf[MU_CIDR_MAXBUFSIZE];
  int rc = mu_cidr_to_string (cidr, flags, buf, sizeof (buf), NULL);
  if (rc)
    return rc;
  *pbuf = strdup (buf);
  if (!*buf)
    return ENOMEM;
  return 0;
}

