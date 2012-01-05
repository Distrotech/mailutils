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
#include <mailutils/errno.h>
#include <mailutils/sys/url.h>

int
mu_url_uplevel (mu_url_t url, mu_url_t *upurl)
{
  int rc;
  char *p;
  mu_url_t new_url;

  if (url->_uplevel)
    return url->_uplevel (url, upurl);

  if (!url->path)
    return MU_ERR_NOENT;
  p = strrchr (url->path, '/');

  rc = mu_url_dup (url, &new_url);
  if (rc == 0)
    {
      if (!p || p == url->path)
	{
	  free (new_url->path);
	  new_url->path = NULL;
	}
      else
	{
	  size_t size = p - url->path;
	  new_url->path = realloc (new_url->path, size + 1);
	  if (!new_url->path)
	    {
	      mu_url_destroy (&new_url);
	      return ENOMEM;
	    }
	  memcpy (new_url->path, url->path, size);
	  new_url->path[size] = 0;
	}
      *upurl = new_url;
    }
  return rc;
}

