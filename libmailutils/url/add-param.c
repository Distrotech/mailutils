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
mu_url_add_param (mu_url_t url, size_t pc, const char **pv)
{
  char **fv;
  int i, j;
    
  if (!url)
    return EINVAL;
  if (!pc || !pv)
    return 0;

  fv = realloc (url->fvpairs,
		sizeof (url->fvpairs[0]) * (url->fvcount + pc + 1));
  if (!fv)
    return ENOMEM;
  url->fvpairs = fv;
  for (i = url->fvcount, j = 0; j < pc; i++, j++)
    {
      fv[i] = strdup (pv[j]);
      if (!fv[i])
	{
	  /* Restore the status quo */
	  for (; j; j--)
	    free (fv[--i]);
	  if (url->fvcount)
	    fv[url->fvcount] = NULL;
	  else
	    {
	      free (url->fvpairs);
	      url->fvpairs = NULL;
	      url->fvcount = 0;
	    }
	  return ENOMEM;
	}
    }
  fv[i] = NULL;
  url->fvcount = i;
  url->flags |= MU_URL_PARAM;
  mu_url_invalidate (url);
  return 0;
}
