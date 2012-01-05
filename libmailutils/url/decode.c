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
#include <mailutils/secret.h>
#include <mailutils/sys/url.h>

struct decode_tab
{
  int mask;
  int (*fun) (mu_url_t, size_t);
  size_t off;
};

static int
_url_dec_str (mu_url_t url, size_t off)
{
  char **pptr = (char**) ((char*) url + off);
  mu_str_url_decode_inline (*pptr);
  return 0;
}

static int
_url_dec_param (mu_url_t url, size_t off)
{
  int i;

  for (i = 0; i < url->fvcount; i++)
    mu_str_url_decode_inline (url->fvpairs[i]);
  return 0;
}

static int
_url_dec_query (mu_url_t url, size_t off)
{
  int i;

  for (i = 0; i < url->qargc; i++)
    mu_str_url_decode_inline (url->qargv[i]);
  return 0;
}

static int
_url_dec_secret (mu_url_t url, size_t off)
{
  char *pass;
  mu_secret_t newsec;
  int rc;

  rc = mu_str_url_decode (&pass, mu_secret_password (url->secret));
  if (rc)
    return rc;
  rc = mu_secret_create (&newsec, pass, strlen (pass));
  memset (pass, 0, strlen (pass));
  free (pass);
  if (rc)
    return rc;
  mu_secret_destroy (&url->secret);
  url->secret = newsec;
  return 0;
}

static struct decode_tab decode_tab[] = {
  { MU_URL_SCHEME, _url_dec_str, mu_offsetof (struct _mu_url, scheme) },
  { MU_URL_USER,   _url_dec_str, mu_offsetof (struct _mu_url, user) },
  { MU_URL_SECRET, _url_dec_secret },
  { MU_URL_AUTH,   _url_dec_str, mu_offsetof (struct _mu_url, auth) },
  { MU_URL_HOST,   _url_dec_str, mu_offsetof (struct _mu_url, host) },
  { MU_URL_PATH,   _url_dec_str, mu_offsetof (struct _mu_url, path) },
  { MU_URL_PARAM,  _url_dec_param, 0 },
  { MU_URL_QUERY,  _url_dec_query, 0 }
};

int
mu_url_decode (mu_url_t url)
{
  int i;

  if (!url)
    return EINVAL;
  for (i = 0; i < MU_ARRAY_SIZE (decode_tab); i++)
    {
      if (url->flags & decode_tab[i].mask)
	{
	  int rc = decode_tab[i].fun (url, decode_tab[i].off);
	  if (rc)
	    return rc;
	}
    }
  return 0;
}

