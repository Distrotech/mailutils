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

#include <stdlib.h>
#include <mailutils/sockaddr.h>

static void
set_next (struct mu_sockaddr *sp, struct mu_sockaddr *tgt)
{
  for (; sp->next; sp = sp->next)
    ;
  sp->next = tgt;
  if (tgt)
    tgt->prev = sp;
}

struct mu_sockaddr *
mu_sockaddr_insert (struct mu_sockaddr *anchor, struct mu_sockaddr *addr,
		    int before)
{
  struct mu_sockaddr *ret = anchor;

  if (!anchor)
    {
      addr->prev = NULL;
      set_next (addr, NULL);
      return addr;
    }
  
  if (before)
    {
      if (anchor->prev)
	anchor = anchor->prev;
      else
	{
	  addr->prev = NULL;
	  set_next (addr, anchor);
	  return addr;
	}
    }

  set_next (addr, anchor->next);
  anchor->next = addr;
  addr->prev = anchor;
  return ret;
}
