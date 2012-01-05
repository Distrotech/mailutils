/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2010, 2012 Free Software Foundation, Inc.

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
#include <mailutils/errno.h>
#include <mailutils/sys/url.h>
#include <mailutils/url.h>

int
mu_url_clear_param (mu_url_t url)
{
  int i;
  
  if (!url)
    return EINVAL;

  for (i = 0; i < url->fvcount; i++)
    free (url->fvpairs[i]);
  free (url->fvpairs);
  url->fvpairs = NULL;
  url->fvcount = 0;
  url->flags &= ~MU_URL_PARAM;
  mu_url_invalidate (url);
  return 0;
}
