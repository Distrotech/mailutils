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
#include <errno.h>
#include <string.h>
#include <mailutils/sockaddr.h>

int
mu_sockaddr_create (struct mu_sockaddr **res,
		    struct sockaddr *addr, socklen_t len)
{
  struct mu_sockaddr *sa;

  sa = calloc (1, sizeof (*sa));
  if (!sa)
    return ENOMEM;
  sa->addr = malloc (len);
  if (!sa->addr)
    {
      free (sa);
      return ENOMEM;
    }
  memcpy (sa->addr, addr, len);
  sa->addrlen = len;
  *res = sa;
  return 0;
}

