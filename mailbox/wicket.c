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
};

static char * stripwhite __P ((char *));
static int myticket_create __P ((ticket_t *, const char *, const char *));
static void myticket_destroy __P ((ticket_t));
static int _get_ticket __P ((ticket_t *, const char *, const char *));
static int myticket_pop __P ((ticket_t, const char *, char **));

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
  if (wicket == NULL || pticket == NULL || user == NULL)
    return EINVAL;

  if (wicket->filename == NULL)
    return EINVAL;

  if (wicket->_get_ticket)
    return wicket->_get_ticket (wicket, user, type, pticket);
  return _get_ticket (pticket, wicket->filename, user);
}

/* FIXME: This is a proof of concept ... write a more intelligent parser.  */
static int
_get_ticket (ticket_t *pticket, const char *filename, const char *user)
{
  FILE *fp;
  char *buf;
  size_t buflen;
  int status = ENOENT;

  fp = fopen (filename, "r");
  if (fp == NULL)
    return errno;

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
		{
		  status = ENOMEM;
		  break;
		}
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
		  sep = stripwhite (sep);
		  status = myticket_create (pticket, ptr, sep);
		  break;
		}
	    }
	  ptr = buf;
	}
    }
  else
    status = ENOMEM;

  if (buf)
    free (buf);
  fclose (fp);
  return status;
}

static int
myticket_create (ticket_t *pticket, const char *user, const char *pass)
{
  struct myticket_data *mdata;
  int status = ticket_create (pticket, NULL);
  if (status != 0)
    return status;

  mdata = calloc (1, sizeof *mdata);
  if (mdata == NULL)
    ticket_destroy (pticket, NULL);
  ticket_set_destroy (*pticket, myticket_destroy, NULL);
  ticket_set_pop (*pticket, myticket_pop, NULL);
  ticket_set_data (*pticket, mdata, NULL);
  if ((mdata->user = strdup (user)) == NULL
      || (mdata->pass = strdup (pass)) == NULL)
    {
      status = ENOMEM;
      ticket_destroy (pticket, NULL);
    }
  return status;
}

static int
myticket_pop (ticket_t ticket, const char *challenge, char **parg)
{
  struct myticket_data *mdata = NULL;
  ticket_get_data (ticket, (void **)&mdata);
  if (challenge && (strstr (challenge, "ass") != NULL
		    || strstr (challenge, "ASS") != NULL))
    *parg = strdup (mdata->pass);
  else
    *parg = strdup (mdata->user);
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
