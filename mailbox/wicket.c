/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2003, 2004 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>

#include <mailutils/errno.h>
#include <mailutils/mutil.h>
#include <mailutils/mu_auth.h>

#include <auth0.h>
#include <url0.h>

struct myticket_data
{
  char *user;
  char *pass;
  char *filename;
};

static int   myticket_create  __P ((ticket_t *, const char *, const char *, const char *));
static void  myticket_destroy __P ((ticket_t));
static int   myticket_pop     __P ((ticket_t, url_t, const char *, char **));
static int   get_pass         __P ((url_t, const char *, const char *, char**));
static int   get_user         __P ((url_t, const char *, char **));

int
wicket_create (wicket_t *pwicket, const char *filename)
{
  struct stat st;

  if (pwicket == NULL)
    return MU_ERR_OUT_PTR_NULL;

  if (filename)
    {
      if (stat (filename, &st) == -1)
	return errno;
      if ((st.st_mode & S_IRWXG) || (st.st_mode & S_IRWXO))
	return MU_ERR_UNSAFE_PERMS;
    }

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
  int e = 0;

  ticket_get_data (ticket, (void **)&mdata);
  if (challenge &&
    (
      strstr (challenge, "ass") != NULL ||
      strstr (challenge, "ASS") != NULL
   )
      )
  {
    if(mdata->pass)
    {
      *parg = strdup(mdata->pass);
      if(!*parg)
	e = ENOMEM;
    }
    else
    {
      e = get_pass (url, mdata->user, mdata->filename, parg);
    }
  }
  else
  {
    if(mdata->user)
    {
      *parg = strdup(mdata->user);
      if(!*parg)
	e = ENOMEM;
    }
    else
    {
      e = get_user (url, mdata->filename, parg);
    }
  }
  return e;
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

static int
get_ticket (url_t url, const char *user, const char *filename, url_t * ticket)
{
  int err = 0;
  FILE *fp = NULL;
  size_t buflen = 128;
  char *buf = NULL;

  if (!filename || !url)
    return EINVAL;

  fp = fopen (filename, "r");

  if (!fp)
    return errno;

  buf = malloc (buflen);

  if (!buf)
    {
      fclose (fp);
      return ENOMEM;
    }

  while (!feof (fp) && !ferror (fp))
    {
      char *ptr = buf;
      int len = 0;
      url_t u = NULL;

      /* fgets:
	 1) return true, read some data
           1a) read a newline, so break
           1b) didn't read newline, so realloc & continue
         2) returned NULL, so no more data
       */
      while (fgets (ptr, buflen, fp) != NULL)
	{
	  char *tmp = NULL;
	  len = strlen (buf);
	  /* Check if a complete line.  */
	  if (len && buf[len - 1] == '\n')
	    break;

	  buflen *= 2;
	  tmp = realloc (buf, buflen);
	  if (tmp == NULL)
	    {
	      free (buf);
	      fclose (fp);
	      return ENOMEM;
	    }
	  buf = tmp;
	  ptr = buf + len;
	}

      len = strlen (buf);

      /* Truncate a trailing newline. */
      if (len && buf[len - 1] == '\n')
	buf[len - 1] = 0;

      /* Skip leading spaces.  */
      ptr = buf;
      while (isspace (*ptr))
	ptr++;

      /* Skip comments. */
      if (*ptr == '#')
	continue;

      /* Skip empty lines. */
      if ((len = strlen (ptr)) == 0)
	continue;

      if ((err = url_create (&u, ptr)) != 0)
	{
	  free (buf);
	  fclose (fp);
	  return err;
	}
      if ((err = url_parse (u)) != 0)
	{
	  /* TODO: send output to the debug stream */
	  /*
	     printf ("url_parse %s failed: [%d] %s\n", str, err, mu_strerror (err));
	   */
	  url_destroy (&u);
	  continue;
	}


      if (!url_is_ticket (u, url))
	{
	  url_destroy (&u);
	  continue;
	}
      /* Also needs to be for name, if we required. */
      if (user)
	{
	  if (u->name && strcmp (u->name, "*") != 0
	      && strcmp (user, u->name) != 0)
	    {
	      url_destroy (&u);
	      continue;
	    }
	}

      /* Looks like a match! */
      *ticket = u;
      u = NULL;
      break;
    }

  fclose (fp);
  free (buf);

  return 0;
}

static int
get_user (url_t url, const char *filename, char **user)
{
  char *u = 0;

  if (url)
    {
      size_t n = 0;
      url_get_user (url, NULL, 0, &n);

      if (n)
	{
	  u = calloc (1, n + 1);
	  url_get_user (url, u, n + 1, NULL);
	}
    }
  if (!u && filename)
    {
      url_t ticket = 0;
      int e = get_ticket (url, NULL, filename, &ticket);
      if (e)
	return e;

      if (ticket)
	{
	  size_t n = 0;
	  url_get_user (ticket, NULL, 0, &n);

	  if (n)
	    {
	      u = calloc (1, n + 1);
	      url_get_user (ticket, u, n + 1, NULL);
	    }
	  url_destroy (&ticket);
	}
    }
  else
    {
      struct mu_auth_data *auth = mu_get_auth_by_uid (getuid ());
      if (auth)
	{
	  u = strdup (auth->name);
	  mu_auth_data_free (auth);
	  if (!u)
	    return ENOMEM;
	}
    }
  *user = u;
  return 0;
}

static int
get_pass (url_t url, const char *user, const char *filename, char **pass)
{
  char *u = 0;

  if (url)
    {
      size_t n = 0;
      url_get_passwd (url, NULL, 0, &n);

      if (n)
	{
	  u = calloc (1, n + 1);
	  url_get_passwd (url, u, n + 1, NULL);
	}
    }
  if (!u && filename)
    {
      url_t ticket = 0;
      int e = get_ticket (url, user, filename, &ticket);
      if (e)
	return e;

      if (ticket)
	{
	  size_t n = 0;
	  url_get_passwd (ticket, NULL, 0, &n);

	  if (n)
	    {
	      u = calloc (1, n + 1);
	      url_get_passwd (ticket, u, n + 1, NULL);
	    }
	  url_destroy (&ticket);
	}
    }
  *pass = u;
  return 0;
}
