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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <url0.h>
#include <registrar0.h>

static void url_pop_destroy (url_t url);

static void
url_pop_destroy (url_t url)
{
  (void)url;
}

/*
  POP URL
  pop://[<user>;AUTH=<auth>@]<host>[:<port>]
*/
int
_url_pop_init (url_t url)
{
  const char *host_port, *indexe;
  char *name = url->name;

  /* reject the obvious */
  if (name == NULL || strncmp (MU_POP_SCHEME, name, MU_POP_SCHEME_LEN) != 0)
    return EINVAL;

  /* do I need to decode url encoding '% hex hex' ? */

  /* TYPE */
  url->_destroy = url_pop_destroy;

  /* SCHEME */
  url->scheme = strdup (MU_POP_SCHEME);
  if (url->scheme == NULL)
    {
      url_pop_destroy (url);
      return ENOMEM;
    }

  name += MU_POP_SCHEME_LEN; /* pass the scheme */

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
      url_pop_destroy (url);
      return -1;
    }
  ((char *)memcpy(url->user, name, indexe - name))[indexe - name] = '\0';

  /* AUTH */
  if (indexe == host_port)
    {
      /* Use default AUTH '*' */
      url->auth = malloc (1 + 1);
      if (url->auth)
	{
	  url->auth[0] = '*';
	  url->auth[1] = '\0';
	}
    }
  else
    {
      /* move pass AUTH= */
      indexe += 6;
      url->auth = malloc (host_port - indexe + 1);
      if (url->auth)
	{
	  ((char *)memcpy (url->auth, indexe, host_port - indexe))
	    [host_port - indexe] = '\0';
	}
    }

  if (url->auth == NULL)
    {
      url_pop_destroy (url);
      return -1;
    }

  /* HOST:PORT */
  if (*host_port == '@')
    host_port++;

  indexe = strchr (host_port, ':');
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
      url_pop_destroy (url);
      return ENOMEM;
    }
  else
    {
      /* playing smart and nuking any trailing slashes on the host */
      size_t len = strlen (url->host);
      if (url->host[len - 1] == '/')
	url->host[len - 1] = '\0'; /* leak a bit */
    }

  return 0;
}
