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
#include <stdlib.h>
#include <string.h>
#include <mailutils/types.h>
#include <mailutils/sys/url.h>
#include <mailutils/errno.h>
#include <mailutils/util.h>
#include <mailutils/opool.h>

#define AUTH_PFX ";AUTH="

static int
url_reconstruct_to_pool (mu_url_t url, mu_opool_t pool)
{
  if (url->flags & MU_URL_SCHEME)
    {
      int i;
      
      mu_opool_appendz (pool, url->scheme);
      mu_opool_append (pool, "://", 3);

      if (url->flags & MU_URL_USER)
	mu_opool_appendz (pool, url->user);
      if (url->flags & MU_URL_SECRET)
	mu_opool_append (pool, ":***", 4); /* FIXME: How about MU_URL_PARSE_HIDEPASS? */
      if (url->flags & MU_URL_AUTH)
	{
	  mu_opool_append (pool, AUTH_PFX, sizeof AUTH_PFX - 1);
	  mu_opool_appendz (pool, url->auth);
	}
      if (url->flags & MU_URL_HOST)
	{
	  if (url->flags & (MU_URL_USER|MU_URL_SECRET|MU_URL_AUTH))
	    mu_opool_append_char (pool, '@');
	  mu_opool_appendz (pool, url->host);
	  if (url->flags & MU_URL_PORT)
	    {
	      mu_opool_append_char (pool, ':');
	      mu_opool_appendz (pool, url->portstr);
	    }
	}
      else if (url->flags & (MU_URL_USER|MU_URL_SECRET|MU_URL_AUTH))
	return MU_ERR_URL_MISS_PARTS;
      
      if (url->flags & MU_URL_PATH)
	{
	  if (url->flags & MU_URL_HOST)
	    mu_opool_append_char (pool, '/');
	  mu_opool_appendz (pool, url->path);
	}
      
      if (url->flags & MU_URL_PARAM)
	{
	  for (i = 0; i < url->fvcount; i++)
	    {
	      mu_opool_append_char (pool, ';');
	      mu_opool_append (pool, url->fvpairs[i],
			       strlen (url->fvpairs[i]));
	    }
	}
      if (url->flags & MU_URL_QUERY)
	{
	  mu_opool_append_char (pool, '?');
	  mu_opool_append (pool, url->qargv[0],
			   strlen (url->qargv[0]));
	  for (i = 1; i < url->qargc; i++)
	    {
	      mu_opool_append_char (pool, '&');
	      mu_opool_append (pool, url->qargv[i],
			       strlen (url->qargv[i]));
	    }
	}
      return 0;
    }
  else if (url->flags == MU_URL_PATH)
    {
      mu_opool_appendz (pool, url->path);
      return 0;
    }
  return MU_ERR_URL_MISS_PARTS;
}

int
mu_url_sget_name (const mu_url_t url, const char **retptr)
{
  if (!url)
    return EINVAL;
  if (!url->name)
    {
      mu_opool_t pool;
      int rc;
      char *ptr, *newname;
      size_t size;
      
      rc = mu_opool_create (&pool, 0);
      if (rc)
	return rc;
      rc = url_reconstruct_to_pool (url, pool);
      if (rc)
	{
	  mu_opool_destroy (&pool);
	  return rc;
	}
      ptr = mu_opool_finish (pool, &size);	  

      newname = realloc (url->name, size + 1);
      if (!newname)
	{
	  mu_opool_destroy (&pool);
	  return ENOMEM;
	}
      memcpy (newname, ptr, size);
      newname[size] = 0;
      url->name = newname;
      mu_opool_destroy (&pool);
    }
  if (retptr)
    *retptr = url->name;
  return 0;
}

int
mu_url_aget_name (const mu_url_t url, char **ret)
{
  char *s;
  const char *ptr;
  int rc = mu_url_sget_name (url, &ptr);
  if (rc)
    return rc;
  s = strdup (ptr);
  if (!s)
    return errno;
  *ret = s;
  return 0;
}

int
mu_url_get_name (const mu_url_t url, char *buf, size_t size, size_t *n)
{
  size_t i;
  const char *ptr;
  int rc = mu_url_sget_name (url, &ptr);
  if (rc)
    return rc;
  i = mu_cpystr (buf, ptr, size);
  if (n)
    *n = i;
  return 0;
}

const char *
mu_url_to_string (const mu_url_t url)
{
  const char *ptr;
  
  if (mu_url_sget_name (url, &ptr))
    return "";
  return ptr;
}
