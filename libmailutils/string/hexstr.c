/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2003, 2005-2007, 2009-2012 Free Software
   Foundation, Inc.

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

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <unistd.h>

/* convert a sequence of hex characters into an integer */

unsigned long
mu_hex2ul (char hex)
{
  if (hex >= '0' && hex <= '9')
   return hex - '0';

  if (hex >= 'a' && hex <= 'z')
    return hex - 'a' + 10;

  if (hex >= 'A' && hex <= 'Z')
    return hex - 'A' + 10;

  return -1;
}

size_t
mu_hexstr2ul (unsigned long *ul, const char *hex, size_t len)
{
  size_t r;

  *ul = 0;

  for (r = 0; r < len; r++)
    {
      unsigned long v = mu_hex2ul (hex[r]);

      if (v == (unsigned long)-1)
	return r;

      *ul = *ul * 16 + v;
    }
  return r;
}
