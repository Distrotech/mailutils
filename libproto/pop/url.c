/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2001, 2003, 2007-2008, 2010-2012 Free Software
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

#ifdef ENABLE_POP

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif

#include <mailutils/sys/url.h>
#include <mailutils/sys/registrar.h>

/*
  POP URLs:
    pop://[<user>[;AUTH=<auth>]@]<host>[:<port>]
    pop://[<user>[:pass]@]<host>[:<port>]
*/

int
_url_pop_init (mu_url_t url)
{
  if (url->port == 0)
    url->port = MU_POP_PORT;
  return 0;
}

/*
  POPS URLs:
    pops://[<user>[;AUTH=<auth>]@]<host>[:<port>]
    pops://[<user>[:pass]@]<host>[:<port>]
*/

int
_url_pops_init (mu_url_t url)
{
  if (url->port == 0)
    url->port = MU_POPS_PORT;
  return 0;
}

#endif /* ENABLE_POP */
