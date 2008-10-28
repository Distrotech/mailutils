/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2007 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library; if not, write to the
   Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301 USA */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#ifdef ENABLE_SENDMAIL

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_PATHS_H
# include <paths.h>
#endif

#ifndef _PATH_SENDMAIL
# define _PATH_SENDMAIL "/usr/lib/sendmail"
#endif

#include <registrar0.h>
#include <url0.h>

/*
  Sendmail URL:
    sendmail:/path/to/sendmail
*/

int
_url_sendmail_init (mu_url_t url)
{
  /* not valid in a sendmail url */
  if (url->user || url->passwd || url->auth || url->qargc
      || url->host || url->port)
    return EINVAL;

  if (url->path == 0)
    if ((url->path = strdup (_PATH_SENDMAIL)) == 0)
      return ENOMEM;

  return 0;
}

#endif /* ENABLE_SENDMAIL */
