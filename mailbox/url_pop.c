/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU Library General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#include <url0.h>
#include <registrar.h>

#include <errno.h>
#include <cpystr.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

static void url_pop_destroy (url_t *purl);
static int url_pop_init (url_t *purl, const char *name);

struct url_registrar _url_pop_registrar =
{
  "pop://",
  url_pop_init, url_pop_destroy
};

static int get_auth (const url_pop_t up, char *s, size_t len, size_t *);

static int
get_auth (const url_pop_t up, char *s, size_t len,  size_t *n)
{
  size_t i;
  if (up)
    return EINVAL;
  i = _cpystr (s, up->auth, len);
  if (n)
    *n = i;
  return 0;
}

int
(url_pop_get_auth) (const url_t url, char *auth, size_t len, size_t *n)
{
  return (url) ?
    ((url_pop_t)(url->data))->_get_auth(url->data, auth, len, n) : EINVAL;
}

static void
url_pop_destroy (url_t *purl)
{
  if (purl && *purl)
    {
      url_t url = *purl;
      free (url->scheme);
      free (url->user);
      free (url->passwd);
      free (url->host);
      if (url->data)
	{
	  url_pop_t up = url->data;
	  free (up->auth);
	}
      free (url->data);
      free (url);
      *purl = NULL;
    }
}

/*
  POP URL
  pop://<user>;AUTH=<auth>@<host>:<port>
*/
static int
url_pop_init (url_t *purl, const char *name)
{
  const char *host_port, *index;
  struct url_registrar *ureg = &_url_pop_registrar;
  size_t len, scheme_len = strlen (ureg->scheme);
  url_t url;
  url_pop_t up;

  /* reject the obvious */
  if (name == NULL || strncmp (ureg->scheme, name, scheme_len) != 0
      || (host_port = strchr (name, '@')) == NULL
      || (len = strlen (name)) < 9 /* 6(scheme)+1(user)+1(@)+1(host)*/)
    return ENOMEM;

  /* do I need to decode url encoding '% hex hex' ? */

  url = calloc(1, sizeof (*url));
  url->data = up = calloc(1, sizeof(*up));
  up->_get_auth = get_auth;

  /* TYPE */
  url->_init = _url_pop_registrar._init;
  url->_destroy = _url_pop_registrar._destroy;

  /* SCHEME */
  url->scheme = strdup ("pop://");
  if (url->scheme == NULL)
    {
      url_pop_destroy (&url);
      return ENOMEM;
    }

  name += scheme_len; /* pass the scheme */

  /* looking for "user;auth=auth-enc" */
  for (index = name; index != host_port; index++)
    {
      /* Auth ? */
      if (*index == ';')
	{
	  /* make sure it the token */
	  if (strncasecmp(index + 1, "auth=", 5) == 0)
	    break;
	}
    }

  if (index == name)
    {
      url_pop_destroy (&url);
      return -1;
    }

  /* USER */
  url->user = malloc(index - name + 1);
  if (url->user == NULL)
    {
      url_pop_destroy (&url);
      return -1;
    }
  ((char *)memcpy(url->user, name, index - name))[index - name] = '\0';

  /* AUTH */
  if ((host_port - index) <= 6 /*strlen(";AUTH=")*/)
    {
      /* Use default AUTH '*' */
      up->auth = malloc (1 + 1);
      if (up->auth)
	{
	  up->auth[0] = '*';
	  up->auth[1] = '\0';
	}
    }
  else
    {
      /* move pass AUTH= */
      index += 6;
      up->auth = malloc (host_port - index + 1);
      if (up->auth)
	{
	  ((char *)memcpy (up->auth, index, host_port - index))
	    [host_port - index] = '\0';
	}
    }

  if (up->auth == NULL)
    {
      url_pop_destroy (&url);
      return -1;
    }

  /* HOST:PORT */
  index = strchr (++host_port, ':');
  if (index == NULL)
    {
      url->host = strdup (host_port);
      url->port = MU_POP_PORT;
    }
  else
    {
      long p = strtol(index + 1, NULL, 10);
      url->host = malloc (index - host_port + 1);
      if (url->host)
	{
	  ((char *)memcpy (url->host, host_port, index - host_port))
	    [index - host_port]='\0';
	  url->port = (p == 0) ? MU_POP_PORT : p;
	}
    }

  if (url->host == NULL)
    {
      url_pop_destroy (&url);
      return ENOMEM;
    }

  *purl = url;
  return 0;
}
