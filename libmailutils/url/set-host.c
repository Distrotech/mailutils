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
mu_url_set_host (mu_url_t url, const char *host)
{
  char *copy;
  
  if (!url)
    return EINVAL;
  if (host)
    {
      size_t len;
      int flag = MU_URL_HOST;
      
      len = strlen (host);
      if (len == 0)
	return EINVAL;
      if (host[0] == '[' && host[len-1] == ']')
	{
	  flag |= MU_URL_IPV6;
	  host++;
	  len -= 2;
	}
      
      copy = malloc (len + 1);
      if (!copy)
	return ENOMEM;
      memcpy (copy, host, len);
      copy[len] = 0;
      url->flags |= flag;
    }
  else
    {
      url->flags &= ~(MU_URL_HOST|MU_URL_IPV6);
      copy = NULL;
    }
  url->_get_host = NULL;
  free (url->host);
  url->host = copy;
  mu_url_invalidate (url);
  return 0;
}
