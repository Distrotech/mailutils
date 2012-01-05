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
#include <string.h>
#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif
#include <stdlib.h>
#include <mailutils/types.h>
#include <mailutils/argcv.h>
#include <mailutils/secret.h>
#include <mailutils/util.h>
#include <mailutils/sys/url.h>

struct copy_tab
{
  int mask;
  int (*fun) (mu_url_t, mu_url_t, size_t);
  size_t off;
};

static int
_url_copy_str (mu_url_t dest_url, mu_url_t src_url, size_t off)
{
  char **dest = (char**) ((char*) dest_url + off);
  char *src = *(char**) ((char*) src_url + off);
  char *p = strdup (src);
  if (!p)
    return ENOMEM;
  *dest = p;
  return 0;
}
  
static int
_url_copy_secret (mu_url_t dest, mu_url_t src, size_t off)
{
  return mu_secret_dup (src->secret, &dest->secret);
}

static int
_url_copy_port (mu_url_t dest, mu_url_t src, size_t off)
{
  if (src->portstr)
    {
      dest->portstr = strdup (src->portstr);
      if (!dest->portstr)
	return ENOMEM;
    }
  dest->port = src->port;
  return 0;
}

static char **
argcv_copy (size_t argc, char **argv)
{
  size_t i;
  char **nv = calloc (argc + 1, sizeof (nv[0]));
  if (!nv)
    return NULL;
  for (i = 0; i < argc; i++)
    if ((nv[i] = strdup (argv[i])) == NULL)
      {
	mu_argcv_free (i, nv);
	free (nv);
	return NULL;
      }
  return nv;
}

static int
_url_copy_param (mu_url_t dest, mu_url_t src, size_t off)
{
  if ((dest->fvpairs = argcv_copy (src->fvcount, src->fvpairs)) == NULL)
    return ENOMEM;
  dest->fvcount = src->fvcount;
  return 0;
}

static int
_url_copy_query (mu_url_t dest, mu_url_t src, size_t off)
{
  if ((dest->qargv = argcv_copy (src->qargc, src->qargv)) == NULL)
    return ENOMEM;
  dest->qargc = src->qargc;
  return 0;
}

static struct copy_tab copy_tab[] = {
  { MU_URL_SCHEME, _url_copy_str, mu_offsetof (struct _mu_url, scheme) },
  { MU_URL_USER,   _url_copy_str, mu_offsetof (struct _mu_url, user) },
  { MU_URL_SECRET, _url_copy_secret, 0 },
  { MU_URL_AUTH,   _url_copy_str, mu_offsetof (struct _mu_url, auth) },
  { MU_URL_HOST,   _url_copy_str, mu_offsetof (struct _mu_url, host) },
  { MU_URL_PORT,   _url_copy_port, 0 },
  { MU_URL_PATH,   _url_copy_str, mu_offsetof (struct _mu_url, path) },
  { MU_URL_PARAM,  _url_copy_param, 0 },
  { MU_URL_QUERY,  _url_copy_query, 0 }
};

int
mu_url_copy_hints (mu_url_t url, mu_url_t hint)
{
  int i;

  if (!url)
    return EINVAL;
  if (!hint)
    return 0;
  for (i = 0; i < MU_ARRAY_SIZE (copy_tab); i++)
    {
      if (!(url->flags & copy_tab[i].mask) &&
	  (hint->flags & copy_tab[i].mask))
	{
	  int rc = copy_tab[i].fun (url, hint, copy_tab[i].off);
	  if (rc)
	    return rc;
	  url->flags |= copy_tab[i].mask;
	}
    }
  return 0;
}

    



