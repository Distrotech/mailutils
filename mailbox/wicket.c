/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

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

#include <errno.h>
#include <sys/types.h>
#include <pwd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>

#include <mailutils/mutil.h>
#include <auth0.h>

struct myticket_data
{
  char *user;
  char *pass;
  char *filename;
};

static char * stripwhite __P ((char *));
static int myticket_create __P ((ticket_t *, const char *, const char *, const char *));
static void myticket_destroy __P ((ticket_t));
static int myticket_pop __P ((ticket_t, url_t, const char *, char **));
static char * get_pass __P ((url_t, const char *, const char *));
static char * get_user __P ((url_t, const char *));

int
wicket_create (wicket_t *pwicket, const char *filename)
{
  if (pwicket == NULL)
    return EINVAL;

  *pwicket = calloc (1, sizeof (**pwicket));
  if (*pwicket == NULL)
    return ENOMEM;

  if (filename)
    (*pwicket)->filename = strdup (filename);
  return 0;
}

void
wicket_destroy (wicket_t *pwicket)
{
  if (pwicket && *pwicket)
    {
      wicket_t wicket = *pwicket;
      if (wicket->filename)
	free (wicket->filename);
      free (wicket);
      *pwicket = NULL;
    }
}

int
wicket_get_filename (wicket_t wicket, char *filename, size_t len,
		     size_t *pwriten)
{
 size_t n;
 if (wicket == NULL)
   return EINVAL;
  n = mu_cpystr (filename, wicket->filename, len);
  if (pwriten)
    *pwriten = n;
  return 0;
}

int
wicket_set_filename (wicket_t wicket, const char *filename)
{
  if (wicket == NULL)
    return EINVAL;

  if (wicket->filename)
    free (wicket->filename);

  wicket->filename = (filename) ? strdup (filename) : NULL;
  return 0;
}

int
wicket_set_ticket (wicket_t wicket, int get_ticket
		   __P ((wicket_t, const char *, const char *, ticket_t *)))
{
  if (wicket == NULL)
    return EINVAL;

  wicket->_get_ticket = get_ticket;
  return 0;
}

int
wicket_get_ticket (wicket_t wicket, ticket_t *pticket, const char *user,
		   const char *type)
{
  if (wicket == NULL || pticket == NULL)
    return EINVAL;

  if (wicket->filename == NULL)
    return EINVAL;

  if (wicket->_get_ticket)
    return wicket->_get_ticket (wicket, user, type, pticket);
  return myticket_create (pticket, user, NULL, wicket->filename);
}

static int
myticket_create (ticket_t *pticket, const char *user, const char *pass, const char *filename)
{
  struct myticket_data *mdata;
  int status = ticket_create (pticket, NULL);
  if (status != 0)
    return status;

  mdata = calloc (1, sizeof *mdata);
  if (mdata == NULL)
    {
      ticket_destroy (pticket, NULL);
      return ENOMEM;
    }

  ticket_set_destroy (*pticket, myticket_destroy, NULL);
  ticket_set_pop (*pticket, myticket_pop, NULL);
  ticket_set_data (*pticket, mdata, NULL);

  if (filename)
    {
      mdata->filename = strdup (filename);
      if (mdata->filename == NULL)
	{
	  ticket_destroy (pticket, NULL);
	  status = ENOMEM;
	  return status;
	}
    }

  if (user)
    {
      mdata->user = strdup (user);
      if (mdata->user == NULL)
	{
	  ticket_destroy (pticket, NULL);
	  status = ENOMEM;
	  return status;
	}
      if (!pass)
	mdata->pass = get_pass (NULL, user, filename);
    }

  if (pass)
    {
      mdata->pass = strdup (pass);
      if (mdata->pass == NULL)
	{
	  ticket_destroy (pticket, NULL);
	  status = ENOMEM;
	  return status;
	}
    }

  return 0;
}

static int
myticket_pop (ticket_t ticket, url_t url, const char *challenge, char **parg)
{
  struct myticket_data *mdata = NULL;
  ticket_get_data (ticket, (void **)&mdata);
  if (challenge && (strstr (challenge, "ass") != NULL
		    || strstr (challenge, "ASS") != NULL))
    *parg = (mdata->pass) ? strdup (mdata->pass) : get_pass (url, mdata->user, mdata->filename);
  else
    *parg = (mdata->user) ? strdup (mdata->user) : get_user (url, mdata->filename);
  return 0;
}

static void
myticket_destroy (ticket_t ticket)
{
  struct myticket_data *mdata = NULL;
  ticket_get_data (ticket, (void **)&mdata);
  if (mdata)
    {
      if (mdata->user)
	free (mdata->user);
      if (mdata->pass)
	free (mdata->pass);
      if (mdata->filename)
	free (mdata->filename);
      free (mdata);
    }
}

/* Strip whitespace from the start and end of STRING.  Return a pointer
   into STRING. */
static char *
stripwhite (char *string)
{
  register char *s, *t;

  for (s = string; isspace (*s); s++)
    ;

  if (*s == 0)
    return (s);

  t = s + strlen (s) - 1;
  while (t > s && isspace (*t))
    t--;
  *++t = '\0';

  return s;
}

static char *
get_user (url_t url, const char *filename)
{
  struct passwd *pw;
  char *u = (char *)"";
  if (url)
    {
      size_t n = 0;
      url_get_user (url, NULL, 0, &n);
      u = calloc (1, n + 1);
      url_get_user (url, u, n + 1, NULL);
      return u;
    }
  else if (filename)
    {
      /* do something.  */
    }
  pw = getpwuid (getuid ());
  if (pw)
    u = pw->pw_name;
  return strdup (u);
}

static char *
get_pass (url_t url, const char *u, const char *filename)
{
  char *user = NULL;
  char *pass = NULL;

  if (u)
    user = strdup (u);
  else if (url)
    {
      size_t n = 0;
      url_get_user (url, NULL, 0, &n);
      user = calloc (1, n + 1);
      url_get_user (url, user, n + 1, NULL);
    }
  else
    user = get_user (NULL, filename);

  if (filename && user)
    {
      FILE *fp;
      fp = fopen (filename, "r");
      if (fp)
	{
	  char *buf;
	  size_t buflen;

	  buflen = 128;
	  buf = malloc (buflen);
	  if (buf)
	    {
	      char *ptr = buf;
	      while (fgets (ptr, buflen, fp) != NULL)
		{
		  size_t len = strlen (buf);
		  char *sep;
		  /* Check if a complete line.  */
		  if (len && buf[len - 1] != '\n')
		    {
		      char *tmp =  realloc (buf, 2*buflen);
		      if (tmp == NULL)
			break;
		      buf = tmp;
		      ptr = buf + len;
		      continue;
		    }

		  ptr = buf;

		  /* Comments.  */
		  if (*ptr == '#')
		    continue;

		  /* Skip leading spaces.  */
		  while (isspace (*ptr))
		    {
		      ptr++;
		      len--;
		    }

		  /* user:passwd.  Separator maybe ": \t" */
		  if (len && ((sep = memchr (ptr, ':', len)) != NULL
			      || (sep = memchr (ptr, ' ', len)) != NULL
			      || (sep = memchr (ptr, '\t', len)) != NULL))
		    {
		      *sep++ = '\0';
		      ptr = stripwhite (ptr);
		      if (strcmp (ptr, user) == 0)
			{
			  pass = strdup (stripwhite (sep));
			  break;
			}
		    }
		  ptr = buf;
		}
	    }

	  if (buf)
	    free (buf);
	  fclose (fp);
	}
    }
  free (user);
  return pass;
}
