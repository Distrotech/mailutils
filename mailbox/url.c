/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Library Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include <mailutils/registrar.h>
#include <misc.h>
#include <url0.h>

int
url_create (url_t *purl, const char *name)
{
  url_t url = calloc(1, sizeof (*url));
  if (url == NULL)
    return ENOMEM;
  url->name = strdup (name);
  *purl = url;
  return 0;
}

void
url_destroy (url_t *purl)
{
  if (purl && *purl)
    {
      url_t url = (*purl);
      if (url->_destroy)
	url->_destroy (url);

      if (url->name)
	free (url->name);

      if (url->scheme)
	free (url->scheme);

      if (url->user)
	free (url->user);

      if (url->passwd)
	free (url->passwd);

      if (url->host)
	free (url->host);

      free (url);

      *purl = NULL;
    }
}

int
url_get_scheme (const url_t url, char *scheme, size_t len, size_t *n)
{
  size_t i;
  if (url == NULL)
    return EINVAL;
  if (url->_get_scheme)
    return url->_get_scheme (url, scheme, len, n);
  i = _cpystr (scheme, url->scheme, len);
  if (n)
    *n = i;
  return 0;
}

int
url_get_user (const url_t url, char *user, size_t len, size_t *n)
{
  size_t i;
  if (url == NULL)
    return EINVAL;
  if (url->_get_user)
    return url->_get_user (url, user, len, n);
  i = _cpystr (user, url->user, len);
  if (n)
    *n = i;
  return 0;
}

/* FIXME: We should not store passwd in clear, but rather
   have a simple encoding, and decoding mechanism */
int
url_get_passwd (const url_t url, char *passwd, size_t len, size_t *n)
{
  size_t i;
  if (url == NULL)
    return EINVAL;
  if (url->_get_passwd)
    return url->_get_passwd (url, passwd, len, n);
  i = _cpystr (passwd, url->passwd, len);
  if (n)
    *n = i;
  return 0;
}

int
url_get_auth (const url_t url, char *auth, size_t len, size_t *n)
{
  size_t i;
  if (url == NULL)
    return EINVAL;
  if (url->_get_auth)
    return url->_get_auth (url, auth, len, n);
  i = _cpystr (auth, url->auth, len);
  if (n)
    *n = i;
  return 0;
}

int
url_get_host (const url_t url, char *host, size_t len, size_t *n)
{
  size_t i;
  if (url == NULL)
    return EINVAL;
  if (url->_get_host)
    return url->_get_host (url, host, len, n);
  i = _cpystr (host, url->host, len);
  if (n)
    *n = i;
  return 0;
}

int
url_get_port (const url_t url, long *pport)
{
  if (url == NULL)
    return EINVAL;
  if (url->_get_port)
    return url->_get_port (url, pport);
  *pport = url->port;
  return 0;
}

int
url_get_path (const url_t url, char *path, size_t len, size_t *n)
{
  size_t i;
  if (url == NULL)
    return EINVAL;
  if (url->_get_path)
    return url->_get_path (url, path, len, n);
  i = _cpystr(path, url->path, len);
  if (n)
    *n = i;
  return 0;
}

int
url_get_query (const url_t url, char *query, size_t len, size_t *n)
{
  size_t i;
  if (url == NULL)
    return EINVAL;
  if (url->_get_query)
    return url->_get_query (url, query, len, n);
  i = _cpystr(query, url->query, len);
  if (n)
    *n = i;
  return 0;
}

const char *
url_to_string (const url_t url)
{
  if (url == NULL || url->name == NULL)
    return "";
  return url->name;
}
