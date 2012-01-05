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
#include <mailutils/util.h>
#include <mailutils/cstr.h>
#include <mailutils/sys/url.h>

/* General accessors: */
#define AC2(a,b) a ## b
#define METHOD(pfx,part) AC2(pfx,part)
#define AC4(a,b,c,d) a ## b ## c ## d
#define ACCESSOR(action,field) AC4(mu_url_,action,_,field)

/* Define a `static get' accessor */
int
ACCESSOR(sget,URL_PART) (mu_url_t url, char const **sptr)
{
  if (url == NULL)
    return EINVAL;
  if (!url->URL_PART)
    {
      if (url->METHOD(_get_,URL_PART))
	{
	  size_t n;
	  char *buf;

	  int status = url->METHOD(_get_,URL_PART) (url, NULL, 0, &n);
	  if (status)
	    return status;

	  buf = malloc (n + 1);
	  if (!buf)
	    return ENOMEM;

	  status = url->METHOD(_get_,URL_PART) (url, buf, n + 1, NULL);
	  if (status)
	    return status;

	  if (buf[0])
	    {
	      /* FIXME */
	      status = mu_str_url_decode (&url->URL_PART, buf);
	      if (status)
		{
		   free (buf);
		   return status;
		}
	    }
	  else
	    url->URL_PART = buf;
	  if (!url->URL_PART)
	    return ENOMEM;
	}
      else
	return MU_ERR_NOENT;
    }
  *sptr = url->URL_PART;
  return 0;
}

/* Define a `get' accessor */
int
ACCESSOR(get,URL_PART) (mu_url_t url, char *buf, size_t len, size_t *n)
{
  size_t i;
  const char *str;
  int status = ACCESSOR(sget, URL_PART) (url, &str);

  if (status)
    return status;

  i = mu_cpystr (buf, str, len);
  if (n)
    *n = i;
  return 0;
}

/* Define an `allocated get' accessor */
int
ACCESSOR(aget, URL_PART) (mu_url_t url, char **buf)
{
  const char *str;
  int status = ACCESSOR(sget, URL_PART) (url, &str);

  if (status)
    return status;

  if (str)
    {
      *buf = strdup (str);
      if (!*buf)
	status = ENOMEM;
    }
  else
    *buf = NULL;
  return status;
}

/* Define a comparator */
int
ACCESSOR(is_same,URL_PART) (mu_url_t url1, mu_url_t url2)
{
  const char *s1, *s2;
  int status1, status2;

  status1 = ACCESSOR(sget, URL_PART) (url1, &s1);
  if (status1 && status1 != MU_ERR_NOENT)
    return 0;
  status2 = ACCESSOR(sget, URL_PART) (url2, &s2);
  if (status2 && status2 != MU_ERR_NOENT)
    return 0;

  if (status1 || status2)
    return status1 == status2; /* Both fields are missing */
  return mu_c_strcasecmp (s1, s2) == 0;
}
