/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2010-2012 Free Software Foundation, Inc.

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

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif

#include <mailutils/types.h>
#include <mailutils/util.h>
#include <mailutils/sys/url.h>

int
mu_url_dup (mu_url_t old_url, mu_url_t *new_url)
{
  int rc;
  const char *s;
  mu_url_t url = calloc (1, sizeof (*url));

  if (!url)
    return ENOMEM;
  mu_url_sget_name (old_url, &s);
  url->name = strdup (s);
  if (!url->name)
    {
      free (url);
      return ENOMEM;
    }
  
  rc = mu_url_copy_hints (url, old_url);
  if (rc)
    {
      mu_url_destroy (&url);
      return rc;
    }
  *new_url = url;
  return 0;
}
