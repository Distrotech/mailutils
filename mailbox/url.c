/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2007 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library; if not, write to the
   Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301 USA */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif

#include <mailutils/mutil.h>
#include <mailutils/errno.h>
#include <url0.h>

/*
  TODO: implement functions to create a url and encode it properly.
*/

static int url_parse0 (mu_url_t, char *);

int
mu_url_create (mu_url_t * purl, const char *name)
{
  mu_url_t url = calloc (1, sizeof (*url));
  if (url == NULL)
    return ENOMEM;

  url->name = strdup (name);
  if (url->name == NULL)
    {
      free (url);
      return ENOMEM;
    }
  *purl = url;
  return 0;
}

void
mu_url_destroy (mu_url_t * purl)
{
  if (purl && *purl)
    {
      mu_url_t url = (*purl);

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

      if (url->auth)
	free (url->auth);

      if (url->host)
	free (url->host);

      if (url->path)
	free (url->path);

      if (url->query)
	free (url->query);

      free (url);

      *purl = NULL;
    }
}

int
mu_url_parse (mu_url_t url)
{
  int err = 0;
  char *n = NULL;
  struct _mu_url u = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

  if (!url || !url->name)
    return EINVAL;

  /* can't have been parsed already */
  if (url->scheme || url->user || url->passwd || url->auth ||
      url->host || url->path || url->query)
    return EINVAL;

  n = strdup (url->name);

  if (!n)
    return ENOMEM;

  err = url_parse0 (&u, n);

  if (!err)
    {
      /* Dup the strings we found. We wouldn't have to do this
         if we did a single alloc of the source url name, and
         kept it around. It's also a good time to do hex decoding,
         though.
       */


#define UALLOC(X) \
  		if(u.X && u.X[0] && (url->X = mu_url_decode(u.X)) == 0) { \
		  err = ENOMEM; \
		  goto CLEANUP; \
		} else { \
		  /* Set zero-length strings to NULL. */ \
		  u.X = NULL; \
		}

      UALLOC (scheme)
	UALLOC (user)
	UALLOC (passwd)
	UALLOC (auth) UALLOC (host) UALLOC (path) UALLOC (query)
#undef UALLOC
	url->port = u.port;
    }

CLEANUP:
  free (n);

  if (err)
    {
#define UFREE(X) if(X) { free(X); X = 0; }

      UFREE (url->scheme)
	UFREE (url->user)
	UFREE (url->passwd)
	UFREE (url->auth)
	UFREE (url->host) UFREE (url->path) UFREE (url->query)
#undef UFREE
    }

  return err;
}

/*

Syntax, condensed from RFC 1738, and extended with the ;auth=
of RFC 2384 (for POP) and RFC 2192 (for IMAP):

url =
    scheme ":" [ "//"

    [ user [ ( ":" password ) | ( ";auth=" auth ) ] "@" ]

    host [ ":" port ]

    [ ( "/" urlpath ) | ( "?" query ) ] ]

All hell will break loose in this parser if the user/pass/auth
portion is missing, and the urlpath has any @ or : characters
in it. A imap mailbox, say, named after the email address of
the person the mail is from:

  imap://imap.uniserve.com/alain@qnx.com

Is this required to be % quoted, though? I hope so!

*/

static int
url_parse0 (mu_url_t u, char *name)
{
  char *p;			/* pointer into name */

  /* reject the obvious */
  if (name == NULL)
    return EINVAL;

  /* Parse out the SCHEME. */
  p = strchr (name, ':');
  if (p == NULL)
    {
      return MU_ERR_PARSE;
    }

  *p++ = 0;

  u->scheme = name;

  /* RFC 1738, section 2.1, lower the scheme case */
  for (; name < p; name++)
    *name = tolower (*name);

  name = p;

  /* Check for nothing following the scheme. */
  if (!*name)
    return 0;

  if (strncmp (name, "//", 2) != 0)
    {
      u->path = name;
      return 0;
    }

  name += 2;

  /* Split into LHS and RHS of the '@', and then parse each side. */
  u->host = strchr (name, '@');
  if (u->host == NULL)
    u->host = name;
  else
    {
      /* Parse the LHS into an identification/authentication pair. */
      *u->host++ = 0;

      u->user = name;

      /* Try to split the user into a:
         <user>:<password>
         or
         <user>;AUTH=<auth>
       */

      for (; *name; name++)
	{
	  if (*name == ';')
	    {
	      /* Make sure it's the auth token. */
	      if (strncasecmp (name + 1, "auth=", 5) == 0)
		{
		  *name++ = 0;

		  name += 5;

		  u->auth = name;

		  break;
		}
	    }
	  if (*name == ':')
	    {
	      *name++ = 0;
	      u->passwd = name;
	      break;
	    }
	}
    }

  /* Parse the host and port from the RHS. */
  p = strchr (u->host, ':');

  if (p)
    {
      *p++ = 0;

      u->port = strtol (p, &p, 10);

      /* Check for garbage after the port: we should be on the start
         of a path, a query, or at the end of the string. */
      if (*p && strcspn (p, "/?") != 0)
	return MU_ERR_PARSE;
    }
  else
    p = u->host + strcspn (u->host, "/?");

  /* Either way, if we're not at a nul, we're at a path or query. */
  if (*p == '?')
    {
      /* found a query */
      *p++ = 0;
      u->query = p;
    }
  if (*p == '/')
    {
      /* found a path */
      *p++ = 0;
      u->path = p;
    }

  return 0;
}

int
mu_url_get_scheme (const mu_url_t url, char *scheme, size_t len, size_t * n)
{
  size_t i;
  if (url == NULL)
    return EINVAL;
  if (url->_get_scheme)
    return url->_get_scheme (url, scheme, len, n);
  i = mu_cpystr (scheme, url->scheme, len);
  if (n)
    *n = i;
  return 0;
}

int
mu_url_get_user (const mu_url_t url, char *user, size_t len, size_t * n)
{
  size_t i;
  if (url == NULL)
    return EINVAL;
  if (url->_get_user)
    return url->_get_user (url, user, len, n);
  i = mu_cpystr (user, url->user, len);
  if (n)
    *n = i;
  return 0;
}

/* FIXME: We should not store passwd in clear, but rather
   have a simple encoding, and decoding mechanism */
int
mu_url_get_passwd (const mu_url_t url, char *passwd, size_t len, size_t * n)
{
  size_t i;
  if (url == NULL)
    return EINVAL;
  if (url->_get_passwd)
    return url->_get_passwd (url, passwd, len, n);
  i = mu_cpystr (passwd, url->passwd, len);
  if (n)
    *n = i;
  return 0;
}

int
mu_url_get_auth (const mu_url_t url, char *auth, size_t len, size_t * n)
{
  size_t i;
  if (url == NULL)
    return EINVAL;
  if (url->_get_auth)
    return url->_get_auth (url, auth, len, n);
  i = mu_cpystr (auth, url->auth, len);
  if (n)
    *n = i;
  return 0;
}

int
mu_url_get_host (const mu_url_t url, char *host, size_t len, size_t * n)
{
  size_t i;
  if (url == NULL)
    return EINVAL;
  if (url->_get_host)
    return url->_get_host (url, host, len, n);
  i = mu_cpystr (host, url->host, len);
  if (n)
    *n = i;
  return 0;
}

int
mu_url_get_port (const mu_url_t url, long *pport)
{
  if (url == NULL)
    return EINVAL;
  if (url->_get_port)
    return url->_get_port (url, pport);
  *pport = url->port;
  return 0;
}

int
mu_url_get_path (const mu_url_t url, char *path, size_t len, size_t * n)
{
  size_t i;
  if (url == NULL)
    return EINVAL;
  if (url->_get_path)
    return url->_get_path (url, path, len, n);
  i = mu_cpystr (path, url->path, len);
  if (n)
    *n = i;
  return 0;
}

int
mu_url_get_query (const mu_url_t url, char *query, size_t len, size_t * n)
{
  size_t i;
  if (url == NULL)
    return EINVAL;
  if (url->_get_query)
    return url->_get_query (url, query, len, n);
  i = mu_cpystr (query, url->query, len);
  if (n)
    *n = i;
  return 0;
}

const char *
mu_url_to_string (const mu_url_t url)
{
  if (url == NULL || url->name == NULL)
    return "";
  return url->name;
}

int
mu_url_is_scheme (mu_url_t url, const char *scheme)
{
  if (url && scheme && url->scheme && strcasecmp (url->scheme, scheme) == 0)
    return 1;

  return 0;
}

int
mu_url_is_same_scheme (mu_url_t url1, mu_url_t url2)
{
  size_t i = 0, j = 0;
  char *s1, *s2;
  int ret = 1;

  mu_url_get_scheme (url1, NULL, 0, &i);
  mu_url_get_scheme (url2, NULL, 0, &j);
  s1 = calloc (i + 1, sizeof (char));
  if (s1)
    {
      mu_url_get_scheme (url1, s1, i + 1, NULL);
      s2 = calloc (j + 1, sizeof (char));
      if (s2)
	{
	  mu_url_get_scheme (url2, s2, j + 1, NULL);
	  ret = !strcasecmp (s1, s2);
	  free (s2);
	}
      free (s1);
    }
  return ret;
}

int
mu_url_is_same_user (mu_url_t url1, mu_url_t url2)
{
  size_t i = 0, j = 0;
  char *s1, *s2;
  int ret = 0;

  mu_url_get_user (url1, NULL, 0, &i);
  mu_url_get_user (url2, NULL, 0, &j);
  s1 = calloc (i + 1, sizeof (char));
  if (s1)
    {
      mu_url_get_user (url1, s1, i + 1, NULL);
      s2 = calloc (j + 1, sizeof (char));
      if (s2)
	{
	  mu_url_get_user (url2, s2, j + 1, NULL);
	  ret = !strcmp (s1, s2);
	  free (s2);
	}
      free (s1);
    }
  return ret;
}

int
mu_url_is_same_path (mu_url_t url1, mu_url_t url2)
{
  size_t i = 0, j = 0;
  char *s1, *s2;
  int ret = 0;

  mu_url_get_path (url1, NULL, 0, &i);
  mu_url_get_path (url2, NULL, 0, &j);
  s1 = calloc (i + 1, sizeof (char));
  if (s1)
    {
      mu_url_get_path (url1, s1, i + 1, NULL);
      s2 = calloc (j + 1, sizeof (char));
      if (s2)
	{
	  mu_url_get_path (url2, s2, j + 1, NULL);
	  ret = !strcmp (s1, s2);
	  free (s2);
	}
      free (s1);
    }
  return ret;
}

int
mu_url_is_same_host (mu_url_t url1, mu_url_t url2)
{
  size_t i = 0, j = 0;
  char *s1, *s2;
  int ret = 0;

  mu_url_get_host (url1, NULL, 0, &i);
  mu_url_get_host (url2, NULL, 0, &j);
  s1 = calloc (i + 1, sizeof (char));
  if (s1)
    {
      mu_url_get_host (url1, s1, i + 1, NULL);
      s2 = calloc (j + 1, sizeof (char));
      if (s2)
	{
	  mu_url_get_host (url2, s2, j + 1, NULL);
	  ret = !strcasecmp (s1, s2);
	  free (s2);
	}
      free (s1);
    }
  return ret;
}

int
mu_url_is_same_port (mu_url_t url1, mu_url_t url2)
{
  long p1 = 0, p2 = 0;

  mu_url_get_port (url1, &p1);
  mu_url_get_port (url2, &p2);
  return (p1 == p2);
}

/* From RFC 1738, section 2.2 */
char *
mu_url_decode (const char *s)
{
  char *d = strdup (s);
  const char *eos = s + strlen (s);
  int i;

  if (!d)
    return NULL;

  for (i = 0; s < eos; i++)
    {
      if (*s != '%')
	{
	  d[i] = *s;
	  s++;
	}
      else
	{
	  unsigned long ul = 0;

	  s++;

	  /* don't check return value, it's correctly coded, or it's not,
	     in which case we just skip the garbage, this is a decoder,
	     not an AI project */

	  mu_hexstr2ul (&ul, s, 2);

	  s += 2;

	  d[i] = (char) ul;
	}
    }

  d[i] = 0;

  return d;
}

static int
defined (const char *s)
{
  if (s && strcmp ("*", s) != 0)
    return 1;
  return 0;
}

int
mu_url_is_ticket (mu_url_t ticket, mu_url_t url)
{
  if (!ticket || !url)
    return 0;

  /* If ticket has a scheme, host, port, or path, then the queries
     equivalent must be defined and match. */
  if (defined (ticket->scheme))
    {
      if (!url->scheme || strcasecmp (ticket->scheme, url->scheme) != 0)
	return 0;
    }
  if (defined (ticket->host))
    {
      if (!url->host || strcasecmp (ticket->host, url->host) != 0)
	return 0;
    }
  if (ticket->port && ticket->port != url->port)
    return 0;
  /* If ticket has a user or pass, but url doesn't thats OK, were
     urling for this info. But if url does have a user/pass, it
     must match the ticket. */
  if (url->user)
    {
      if (defined (ticket->user) && strcmp (ticket->user, url->user) != 0)
	return 0;
    }

  /* Guess it matches. */
  return 1;
}
