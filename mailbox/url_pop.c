/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU Library General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif

#include <url0.h>
#include <registrar0.h>

static void url_pop_destroy (url_t url);

static void
url_pop_destroy (url_t url)
{
  (void)url;
}

/*
  POP URL:
    pop://[<user>[;AUTH=<auth>]@]<host>[:<port>]
  or:
    pop://[<user>[:pass]@]<host>[:<port>]
*/

int
_url_pop_init (url_t url)
{
  int status = 0;

  url->_destroy = url_pop_destroy;

  status = url_parse(url);

  if(status)
    return status;

  /* is it pop? */
  if (strcmp ("pop", url->scheme) != 0)
    return EINVAL;

  /* not valid in a pop url */
  if (url->path || url->query)
    return EINVAL;

  if (url->port == 0)
    url->port = MU_POP_PORT;
 
  return status;
}

