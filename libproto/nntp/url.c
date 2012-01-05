/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2004, 2007, 2010-2012 Free Software Foundation, Inc.

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

#ifdef ENABLE_NNTP

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif

#include <mailutils/nntp.h>

#include <mailutils/sys/url.h>

/*
  POP URL:
  nntp://<host>:<port>/<newsgroup-name>/<article-number>
*/

int
_nntp_url_init (mu_url_t url)
{
  if (url->port == 0)
    url->port = MU_NNTP_DEFAULT_PORT;

  return status;
}

#endif
