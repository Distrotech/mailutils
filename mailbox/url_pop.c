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
#include <stdlib.h>
#include <string.h>
#include <errno.h>

static void url_pop_destroy (url_t *purl);
static int url_pop_create (url_t *purl, const char *name);

struct url_registrar _url_pop_registrar =
{
  "pop://",
  url_pop_create, url_pop_destroy
};

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
      free (url);
      *purl = NULL;
    }
}

/*
  POP URL
  pop://[<user>;AUTH=<auth>@]<host>[:<port>]
*/
static int
url_pop_create (url_t *purl, const char *name)
{
  const char *host_port, *indexe;
  struct url_registrar *ureg = &_url_pop_registrar;
  size_t scheme_len = strlen (ureg->scheme);
  url_t url;

  /* reject the obvious */
  if (name == NULL || strncmp (ureg->scheme, name, scheme_len) != 0)
    return EINVAL;

  /* do I need to decode url encoding '% hex hex' ? */

  url = calloc(1, sizeof (*url));
  if (url == NULL)
    return ENOMEM;

  /* TYPE */
  url->_create = _url_pop_registrar._create;
  url->_destroy = _url_pop_registrar._destroy;

  /* SCHEME */
  url->scheme = strdup ("pop://");
  if (url->scheme == NULL)
    {
      url_pop_destroy (&url);
      return ENOMEM;
    }

  name += scheme_len; /* pass the scheme */

  host_port = strchr (name, '@');
  if (host_port == NULL)
    host_port= name;

  /* looking for "user;auth=auth-enc" */
  for (indexe = name; indexe != host_port; indexe++)
    {
      /* Auth ? */
      if (*indexe == ';')
	{
	  /* make sure it's the token */
	  if (strncasecmp(indexe + 1, "auth=", 5) == 0)
	    break;
	}
    }

  /* USER */
  url->user = malloc(indexe - name + 1);
  if (url->user == NULL)
    {
      url_pop_destroy (&url);
      return -1;
    }
  ((char *)memcpy(url->user, name, indexe - name))[indexe - name] = '\0';

  /* AUTH */
  if (indexe == host_port)
    {
      /* Use default AUTH '*' */
      url->passwd = malloc (1 + 1);
      if (url->passwd)
	{
	  url->passwd[0] = '*';
	  url->passwd[1] = '\0';
	}
    }
  else
    {
      /* move pass AUTH= */
      indexe += 6;
      url->passwd = malloc (host_port - indexe + 1);
      if (url->passwd)
	{
	  ((char *)memcpy (url->passwd, indexe, host_port - indexe))
	    [host_port - indexe] = '\0';
	}
    }

  if (url->passwd == NULL)
    {
      url_pop_destroy (&url);
      return -1;
    }

  /* HOST:PORT */
  indexe = strchr (++host_port, ':');
  if (indexe == NULL)
    {
      url->host = strdup (host_port);
      url->port = MU_POP_PORT;
    }
  else
    {
      long p = strtol(indexe + 1, NULL, 10);
      url->host = malloc (indexe - host_port + 1);
      if (url->host)
	{
	  ((char *)memcpy (url->host, host_port, indexe - host_port))
	    [indexe - host_port]='\0';
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
