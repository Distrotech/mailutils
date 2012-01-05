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
#include <mailutils/sockaddr.h>
#include <mailutils/cctype.h>

int 
mu_str_is_ipv4 (const char *addr)
{
  int dot_count = 0;
  int digit_count = 0;

  for (; *addr; addr++)
    {
      if (!mu_isascii (*addr))
	return 0;
      if (*addr == '.')
	{
	  if (++dot_count > 3)
	    break;
	  digit_count = 0;
	}
      else if (!(mu_isdigit (*addr) && ++digit_count <= 3))
	return 0;
    }

  return (dot_count == 3);
}

int 
mu_str_is_ipv6 (const char *addr)
{
  int col_count = 0; /* Number of colons */
  int dcol = 0;      /* Did we encounter a double-colon? */
  int dig_count = 0; /* Number of digits in the last group */
	
  for (; *addr; addr++)
    {
      if (!mu_isascii (*addr))
	return 0;
      else if (mu_isxdigit (*addr))
	{
	  if (++dig_count > 4)
	    return 0;
	}
      else if (*addr == ':')
	{
	  if (col_count && dig_count == 0 && ++dcol > 1)
	    return 0;
	  if (++col_count > 7)
	    return 0;
	  dig_count = 0;
	}
      else
	return 0;
    }
  
  return (col_count == 7 || dcol);
}

int
mu_str_is_ipaddr (const char *addr)
{
  if (strchr (addr, '.'))
    return mu_str_is_ipv4(addr);
  else if (strchr (addr, ':'))
    return mu_str_is_ipv6(addr);
  return 0;
}
