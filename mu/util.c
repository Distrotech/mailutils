/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2010-2012 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>. */

#if defined(HAVE_CONFIG_H)
# include <config.h>
#endif

#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <mailutils/mailutils.h>
#include "mu.h"

int
port_from_sa (struct mu_sockaddr *sa)
{
  switch (sa->addr->sa_family)
    {
    case AF_INET:
      return ntohs (((struct sockaddr_in *)sa->addr)->sin_port);

#ifdef MAILUTILS_IPV6
    case AF_INET6:
      return ntohs (((struct sockaddr_in6 *)sa->addr)->sin6_port);
#endif
    }
  return 0;
}

