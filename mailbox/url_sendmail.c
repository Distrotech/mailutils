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

static void url_sendmail_destroy (url_t purl);

static void
url_sendmail_destroy (url_t url)
{
  (void) url;
}

/*
  Sendmail URL:
    sendmail:/path/to/sendmail
*/

int
_url_sendmail_init (url_t url)
{
  int status = 0;

  url->_destroy = url_sendmail_destroy;

  status = url_parse(url);

  if(status)
    return status;

  /* is it sendmail? */
  if (strcmp ("sendmail", url->scheme) != 0)
    return EINVAL;

  /* not valid in a sendmail url */
  if (url->user || url->passwd || url->auth || url->query
      || url->host || url->port)
    return EINVAL;

  if (url->path == 0)
    if((url->path = strdup(_PATH_SENDMAIL)) == 0)
      status = ENOMEM;

  return status;
}

#endif
