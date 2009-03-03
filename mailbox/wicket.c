/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2003, 2004, 
   2005, 2007 Free Software Foundation, Inc.

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

static int   myticket_create  (mu_ticket_t *, const char *, const char *, const char *);
static void  myticket_destroy (mu_ticket_t);
static int   myticket_pop     (mu_ticket_t, mu_url_t, const char *, char **);
static int   get_pass         (mu_url_t, const char *, const char *, char**);
static int   get_user         (mu_url_t, const char *, char **);

int
mu_wicket_create (mu_wicket_t *pwicket, const char *filename)
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
mu_wicket_destroy (mu_wicket_t *pwicket)
{
  if (pwicket && *pwicket)
    {
      mu_wicket_t wicket = *pwicket;
      if (wicket->filename)
	free (wicket->filename);
      free (wicket);
      *pwicket = NULL;
    }
}

int
mu_wicket_get_filename (mu_wicket_t wicket, char *filename, size_t len,
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
mu_wicket_set_filename (mu_wicket_t wicket, const char *filename)
{
  if (wicket == NULL)
    return EINVAL;
  
  if (wicket->filename)
    free (wicket->filename);
  
  wicket->filename = (filename) ? strdup (filename) : NULL;
  return 0;
}

int
mu_wicket_set_ticket (mu_wicket_t wicket, int (*get_ticket)
		      (mu_wicket_t, const char *, const char *, mu_ticket_t *))
{
  if (wicket == NULL)
    return EINVAL;

  wicket->_get_ticket = get_ticket;
  return 0;
}

int
mu_wicket_get_ticket (mu_wicket_t wicket, mu_ticket_t *pticket,
		      const char *user,
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
myticket_create (mu_ticket_t *pticket, const char *user,
		 const char *pass, const char *filename)
{
  struct myticket_data *mdata;
  int status = mu_ticket_create (pticket, NULL);
  if (status != 0)
    return status;

  mdata = calloc (1, sizeof *mdata);
  if (mdata == NULL)
    {
      mu_ticket_destroy (pticket, NULL);
      return ENOMEM;
    }

  mu_ticket_set_destroy (*pticket, myticket_destroy, NULL);
  mu_ticket_set_pop (*pticket, myticket_pop, NULL);
  mu_ticket_set_data (*pticket, mdata, NULL);

  if (filename)
    {
      mdata->filename = strdup (filename);
      if (mdata->filename == NULL)
	{
	  mu_ticket_destroy (pticket, NULL);
	  status = ENOMEM;
	  return status;
	}
    }

  if (user)
    {
      mdata->user = strdup (user);
      if (mdata->user == NULL)
	{
	  mu_ticket_destroy (pticket, NULL);
	  status = ENOMEM;
	  return status;
	}
    }

  if (pass)
    {
      mdata->pass = strdup (pass);
      if (mdata->pass == NULL)
	{
	  mu_ticket_destroy (pticket, NULL);
	  status = ENOMEM;
	  return status;
	}
    }

  return 0;
}

static int
myticket_pop (mu_ticket_t ticket, mu_url_t url,
	      const char *challenge, char **parg)
{
  struct myticket_data *mdata = NULL;
  int e = 0;
  
  mu_ticket_get_data (ticket, (void **)&mdata);
  if (challenge && (strstr (challenge, "ass") != NULL ||
		    strstr (challenge, "ASS") != NULL))
    {
      if (mdata->pass)
	{
	  *parg = strdup (mdata->pass);
	  if (!*parg)
	    e = ENOMEM;
	}
      else
	e = get_pass (url, mdata->user, mdata->filename, parg);
    }
  else
    {
      if (mdata->user)
	{
	  *parg = strdup(mdata->user);
	  if (!*parg)
	    e = ENOMEM;
	}
      else
	e = get_user (url, mdata->filename, parg);
  }
  return e;
}

static void
myticket_destroy (mu_ticket_t ticket)
{
  struct myticket_data *mdata = NULL;
  mu_ticket_get_data (ticket, (void **)&mdata);
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
get_ticket (mu_url_t url, const char *user, const char *filename,
	    mu_url_t * ticket)
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
      mu_url_t u = NULL;

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

      if ((err = mu_url_create (&u, ptr)) != 0)
	{
	  free (buf);
	  fclose (fp);
	  return err;
	}
      if ((err = mu_url_parse (u)) != 0)
	{
	  /* TODO: send output to the debug stream */
	  /*
	     printf ("mu_url_parse %s failed: [%d] %s\n", str, err, mu_strerror (err));
	   */
	  mu_url_destroy (&u);
	  continue;
	}


      if (!mu_url_is_ticket (u, url))
	{
	  mu_url_destroy (&u);
	  continue;
	}
      /* Also needs to be for name, if we required. */
      if (user)
	{
	  if (u->name && strcmp (u->name, "*") != 0
	      && strcmp (user, u->name) != 0)
	    {
	      mu_url_destroy (&u);
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
get_user (mu_url_t url, const char *filename, char **user)
{
  char *u = 0;
  int status;

  if (url)
    {
      status = mu_url_aget_user (url, &u);
      if (status && status != MU_ERR_NOENT)
        return status;
    }

  if (!u && filename)
    {
      mu_url_t ticket = 0;
      status = get_ticket (url, NULL, filename, &ticket);
      if (status)
	return status;

      if (ticket)
	{
	  status = mu_url_aget_user (ticket, &u);
          if (status && status != MU_ERR_NOENT)
            return status;
	  mu_url_destroy (&ticket);
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
get_pass (mu_url_t url, const char *user, const char *filename, char **pass)
{
  int status;
  char *u = 0;

  if (url)
    {
      status = mu_url_aget_passwd (url, &u);
      if (status && status != MU_ERR_NOENT)
	return status;
    }

  if (!u && filename)
    {
      mu_url_t ticket = 0;
      status = get_ticket (url, user, filename, &ticket);
      if (status)
	return status;

      if (ticket)
	{
	  status = mu_url_aget_passwd (ticket, &u);
          if (status && status != MU_ERR_NOENT)
	    return status;
	  mu_url_destroy (&ticket);
	}
    }
  *pass = u;
  return 0;
}
