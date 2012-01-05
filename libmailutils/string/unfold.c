/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2009-2012 Free Software Foundation, Inc.

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
#include <errno.h>

int
mu_string_unfold (char *text, size_t *plen)
{
  char *p, *q;
  enum uf_state { uf_init, uf_nl, uf_fold } state = uf_init;
#define ISSPACE(c) (c == '\r' || c == ' ' || c == '\t')
  
  if (!text)
    return EINVAL;
  
  for (p = q = text; *q; q++)
    {
      switch (state)
	{
	case uf_init:
	  if (*q == '\n')
	    state = uf_nl;
	  else
	    *p++ = *q;
	  break;

	case uf_nl:
	  if (ISSPACE (*q))
	    state = uf_fold;
	  else
	    {
	      state = uf_init;
	      *p++ = *q;
	    }
	  break;

	case uf_fold:
	  if (!ISSPACE (*q))
	    {
	      *p++ = ' ';
	      *p++ = *q;
	      state = uf_init;
	    }
	  break;
	}
    }
  
  *p++ = 0;
  if (plen)
    *plen = p - text;
  return 0;
}
