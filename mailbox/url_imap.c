/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

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
#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif

#include <registrar0.h>
#include <url0.h>

static void url_imap_destroy (url_t url);

static void
url_imap_destroy (url_t url)
{
  (void)url;
}

/*
  IMAP URL
  imap://[<user>;AUTH=<auth>@]<host>/
*/
int
_url_imap_init (url_t url)
{
  const char *host, *indexe;
  char *name = url->name;

  /* reject the obvious */
  if (name == NULL || strncmp (MU_IMAP_SCHEME, name, MU_IMAP_SCHEME_LEN) != 0)
    return EINVAL;

  /* do I need to decode url encoding '% hex hex' ? */

  /* TYPE */
  url->_destroy = url_imap_destroy;

  /* SCHEME */
  url->scheme = strdup (MU_IMAP_SCHEME);
  if (url->scheme == NULL)
    {
      url_imap_destroy (url);
      return ENOMEM;
    }

  name += MU_IMAP_SCHEME_LEN; /* pass the scheme */

  host = strchr (name, '@');
  if (host == NULL)
    host= name;

  /* looking for ";auth=auth-enc" */
  for (indexe = name; indexe != host; indexe++)
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
      url_imap_destroy (url);
      return -1;
    }
  ((char *)memcpy(url->user, name, indexe - name))[indexe - name] = '\0';

  /* AUTH */
  if (indexe == host)
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
      url->auth = malloc (host - indexe + 1);
      if (url->auth)
	{
	  ((char *)memcpy (url->auth, indexe, host - indexe))
	    [host - indexe] = '\0';
	}
    }

  if (url->auth == NULL)
    {
      url_imap_destroy (url);
      return -1;
    }

  /* HOST*/
  if (*host == '@')
    host++;

  indexe = strchr (host, '/');
  if (indexe == NULL)
    url->host = strdup (host);
  else
    {
      char *question;
      url->host = malloc (indexe - host + 1);
      if (url->host)
	((char *)memcpy (url->host, host, indexe - host))[indexe - host] = '\0';
      indexe++;
      /* The query starts after a '?'.  */
      question = strchr (indexe, '?');
      if (question == NULL)
	url->path = strdup (indexe);
      else
	{
	  url->path = malloc (question - indexe + 1);
	  if (url->path)
	    ((char *)memcpy (url->path, indexe,
			     question - indexe))[question - indexe] = '\0';
	  url->query = strdup (question);
	}
    }

  if (url->host == NULL)
    {
      url_imap_destroy (url);
      return ENOMEM;
    }

  url->port = MU_IMAP_PORT;
  return 0;
}
