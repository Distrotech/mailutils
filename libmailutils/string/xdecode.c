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

#include <string.h>
#include <mailutils/errno.h>
#include <mailutils/util.h>

/* From RFC 1738, section 2.2 */
void
mu_str_url_decode_inline (char *s)
{
  char *d;

  d = strchr (s, '%');
  if (!d)
    return;

  for (s = d; *s; )
    {
      if (*s != '%')
	{
	  *d++ = *s++;
	}
      else
	{
	  unsigned long ul = 0;

	  s++;

	  /* don't check return value, it's correctly coded, or it's not,
	     in which case we just skip the garbage, this is a decoder,
	     not an AI project */

	  mu_hexstr2ul (&ul, s, 2);

	  s += 2;

	  *d++ = (char) ul;
	}
    }

  *d = 0;
}

int
mu_str_url_decode (char **ptr, const char *s)
{
  char *d = strdup (s);
  if (!d)
    return ENOMEM;
  mu_str_url_decode_inline (d);
  *ptr = d;
  return 0;
}
