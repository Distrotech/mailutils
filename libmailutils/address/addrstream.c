/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2001, 2005-2007, 2009-2012 Free Software
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <stdlib.h>
#include <mailutils/address.h>
#include <mailutils/stream.h>

int
mu_stream_format_address (mu_stream_t str, mu_address_t addr)
{
  int comma = 0;

  for (;addr; addr = addr->next)
    {
      mu_validate_email (addr);
      if (addr->email)
	{
	  int space = 0;

	  if (comma)
	    mu_stream_write (str, ",", 1, NULL);

	  if (addr->personal)
	    {
	      mu_stream_printf (str, "\"%s\"", addr->personal);
	      space++;
	    }

	  if (addr->comments)
	    {
	      if (space)
		mu_stream_write (str, " ", 1, NULL);
	      mu_stream_printf (str, "(%s)", addr->comments);
	      space++;
	    }

	  if (space)
	    mu_stream_write (str, " ", 1, NULL);
	  mu_stream_printf (str, "<%s>", addr->email);
	  comma++;
	}
    }
  return mu_stream_err (str) ? mu_stream_last_error (str) : 0;
}
