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

#include <url0.h>
#include <registrar0.h>
#include <cpystr.h>

#include <string.h>
#include <stdlib.h>
#include <errno.h>


/* Forward prototypes */
static int get_scheme  (const url_t, char *, size_t, size_t *);
static int get_user    (const url_t, char *, size_t, size_t *);
static int get_passwd  (const url_t, char *, size_t, size_t *);
static int get_host    (const url_t, char *, size_t, size_t *);
static int get_port    (const url_t, long *);
static int get_path    (const url_t, char *, size_t, size_t *);
static int get_query   (const url_t, char *, size_t, size_t *);
static int get_id      (const url_t, int *);

int
url_init (url_t * purl, const char *name)
{
  int status = EINVAL;
  struct url_registrar *ureg;
  struct mailbox_registrar *mreg;
  size_t name_len;
  int id;
  size_t i, entry_count = 0;

  /* Sanity checks */
  if (name == NULL || *name == '\0')
    return status;

  name_len = strlen (name);

  /* Search for a known scheme */
  registrar_entry_count (&entry_count);
  for (i = 0; i < entry_count; i++)
    {
      if (registrar_entry (i, &ureg, &mreg, &id) == 0)
	{
	  size_t scheme_len;
	  if (ureg && ureg->scheme &&
	      name_len > (scheme_len = strlen (ureg->scheme)) &&
	      memcmp (name, ureg->scheme, scheme_len) == 0)
	    {
	      status = 0;
	      break;
	    }
	}
    }
  /*
  while (registrar_list (&ureg, &mreg, &id, &reg) == 0)
    {
      size_t scheme_len;
      if (ureg && ureg->scheme &&
	  name_len > (scheme_len = strlen (ureg->scheme)) &&
	  memcmp (name, ureg->scheme, scheme_len) == 0)
	{
	  status = 0;
	  break;
	}
    }
  */

  /* Found one initialize it */
  if (status == 0)
    {
      status = ureg->_init (purl, name);
      if (status == 0)
	{
	  url_t url = *purl;
	  url->id = id;
	  if (url->_get_id == NULL)
	    url->_get_id = get_id;
	  if (url->_get_scheme == NULL)
	    url->_get_scheme = get_scheme;
	  if (url->_get_user == NULL)
	    url->_get_user = get_user;
	  if (url->_get_passwd == NULL)
	    url->_get_passwd = get_passwd;
	  if (url->_get_host == NULL)
	    url->_get_host = get_host;
	  if (url->_get_port == NULL)
	    url->_get_port = get_port;
	  if (url->_get_path == NULL)
	    url->_get_path = get_path;
	  if (url->_get_query == NULL)
	    url->_get_query = get_query;
	}
    }
  return status;
}

void
url_destroy (url_t *purl)
{
  if (purl && *purl)
    {
      struct url_registrar *ureg;
      int id;
      url_get_id (*purl, &id);
      registrar_get (id, &ureg, NULL);
      ureg->_destroy(purl);
      (*purl) = NULL;
    }
}

int (url_get_scheme) (const url_t url, char *scheme, size_t len, size_t *n)
{
  return (url) ? url->_get_scheme(url, scheme, len, n) : EINVAL;
}

int (url_get_user) (const url_t url, char *user, size_t len, size_t *n)
{
  return (url) ? url->_get_user(url, user, len, n) : EINVAL;
}

int (url_get_passwd) (const url_t url, char *passwd, size_t len, size_t *n)
{
  return (url) ? url->_get_passwd(url, passwd, len, n) : EINVAL;
}

int (url_get_host) (const url_t url, char *host, size_t len, size_t *n)
{
  return (url) ? url->_get_host(url, host, len, n) : EINVAL;
}

int (url_get_port) (const url_t url, long *port)
{
  return (url) ? url->_get_port(url, port) : EINVAL;
}

int (url_get_path) (const url_t url, char *path, size_t len, size_t *n)
{
  return (url) ? url->_get_path(url, path, len, n) : EINVAL;
}

int (url_get_query) (const url_t url, char *query, size_t len, size_t *n)
{
  return (url) ? url->_get_query(url, query, len, n) : EINVAL;
}

int (url_get_id) (const url_t url, int *id)
{
  return (url) ? url->_get_id (url, id) : EINVAL;
}

/* Simple stub functions they all call _cpystr */

static int
get_scheme (const url_t u, char *s, size_t len, size_t *n)
{
  size_t i;
  if (u == NULL)
    return EINVAL;
  i = _cpystr (s, u->scheme, len);
  if (n)
    *n = i;
  return 0;
}

static int
get_user (const url_t u, char *s, size_t len, size_t *n)
{
  size_t i;
  if (u == NULL)
    return EINVAL;
  i = _cpystr (s, u->user, len);
  if (n)
    *n = i;
  return 0;
}

/* FIXME: We should not store passwd in clear, but rather
   have a simple encoding, and decoding mechanism */
static int
get_passwd (const url_t u, char *s, size_t len, size_t *n)
{
  size_t i;
  if (u == NULL)
    return EINVAL;
  i = _cpystr (s, u->passwd, len);
  if (n)
    *n = i;
  return 0;
}

static int
get_host (const url_t u, char *s, size_t len, size_t *n)
{
  size_t i;
  if (u == NULL)
    return EINVAL;
  i = _cpystr (s, u->host, len);
  if (n)
    *n = i;
  return 0;
}

static int
get_port (const url_t u, long * p)
{
  *p = u->port;
  return 0;
}

static int
get_path (const url_t u, char *s, size_t len, size_t *n)
{
  size_t i;
  if (u == NULL)
    return EINVAL;
  i = _cpystr(s, u->path, len);
  if (n)
    *n = i;
  return 0;
}

static int
get_query (const url_t u, char *s, size_t len, size_t *n)
{
  size_t i;
  if (u == NULL)
    return EINVAL;
  i = _cpystr(s, u->query, len);
  if (n)
    *n = i;
  return 0;
}

static int
get_id (const url_t u, int *id)
{
  if (id)
    *id = u->id;
  return 0;
}
