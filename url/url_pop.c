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

#include <url_pop.h>
#include <cpystr.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

struct url_type _url_pop_type =
{
  "pop://", 6,
  "pop://<user>;AUTH=<auth>@<hostname>:<port>",
  (int)&_url_pop_type,
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
  return (url) ? ((url_pop_t)(url->data))->_get_auth(url->data, auth, len, n)
    : EINVAL;
}

void
url_pop_destroy (url_t *purl)
{
  if (purl && *purl)
    {
      url_t url = *purl;
      if (url->scheme)
	free (url->scheme);
      if (url->user)
	free (url->user);
      if (url->passwd)
	free (url->passwd);
      if (url->host)
	free (url->host);
      if (url->data)
	{
	  url_pop_t up = url->data;
	  if (up->auth)
	    free (up->auth);
	  free (url->data);
	}
      free (url);
      url = NULL;
    }
}

/*
  POP URL
  pop://<user>;AUTH=<auth>@<host>:<port>
*/
int
url_pop_init (url_t *purl, const char *name)
{
  const char *host_port, *index;
  url_t url;
  url_pop_t up;

  /* reject the obvious */
  if (name == NULL || strncmp ("pop://", name, 6) != 0
      || (host_port = strchr (name, '@')) == NULL
      || strlen(name) < 9 /* 6(scheme)+1(user)+1(@)+1(host)*/)
    {
      return -1;
    }

  /* do I need to decode url encoding '% hex hex' ? */

  url = calloc(1, sizeof (*url));
  url->data = up = calloc(1, sizeof(*up));
  up->_get_auth = get_auth;

  /* TYPE */
  url->utype = &_url_pop_type;

  /* SCHEME */
  url->scheme = malloc (6 + 1);
  if (url->scheme == NULL)
    {
      url_pop_destroy (&url);
      return -1;
    }
  memcpy (url->scheme, "pop://", 7);

  name += 6; /* pass the scheme */

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
      int len = strlen (host_port);
      url->host = malloc (len + 1);
      if (url->host)
	{
	  ((char *)memcpy (url->host, host_port, len))[len] = '\0';
	  url->port = MU_POP_PORT;
	}
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
      return -1;
    }

  *purl = url;
  return 0;
}
