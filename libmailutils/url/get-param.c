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
#include <mailutils/argcv.h>
#include <mailutils/sys/url.h>

/* field-value pairs accessors */
int
mu_url_sget_fvpairs (const mu_url_t url, size_t *fvc, char ***fvp)
{
  if (url == NULL)
    return EINVAL;
  /* FIXME: no _get_fvpairs method, but the method stuff needs to be rewritten
     anyway */
  *fvc = url->fvcount;
  *fvp = url->fvpairs;
  return 0;
}

int
mu_url_sget_param (const mu_url_t url, const char *param, const char **val)
{
  size_t fvc;
  char **fvp;
  int status = mu_url_sget_fvpairs (url, &fvc, &fvp);

  if (status)
    return status;

  if (fvc)
    {
      size_t i;
      
      for (i = 0; i < fvc; i++)
	{
	  const char *p;
	  char *q;

	  for (p = param, q = fvp[i]; *p && *q && *p == *q; p++, q++)
	    ;
	  if (*p == 0)
	    {
	      if (*q == 0)
		{
		  if (val)
		    *val = q;
		  return 0;
		}
	      else if (*q == '=')
		{
		  if (val)
		    *val = q + 1;
		  return 0;
		}
	    }
	}
    }
 
  return MU_ERR_NOENT;
}
  
int
mu_url_aget_fvpairs (const mu_url_t url, size_t *pfvc, char ***pfvp)
{
  size_t fvc, i;
  char **fvp;
  char **fvcopy;

  int rc = mu_url_sget_fvpairs (url, &fvc, &fvp);
  if (rc)
    return rc;

  fvcopy = calloc (fvc + 1, sizeof (fvcopy[0]));
  if (!fvcopy)
    return errno;
  for (i = 0; i < fvc; i++)
    {
      if (!(fvcopy[i] = strdup (fvp[i])))
	{
	  mu_argcv_free (i, fvcopy);
	  return errno;
	}
    }
  fvcopy[i] = NULL;
  *pfvc = fvc;
  *pfvp = fvcopy;
  return 0;
}

int
mu_url_aget_param (const mu_url_t url, const char *param, char **val)
{
  const char *s;
  int status = mu_url_sget_param (url, param, &s);

  if (status == 0)
    {
      *val = strdup (s);
      if (!*val)
	status = ENOMEM;
    }
  return status;
}

