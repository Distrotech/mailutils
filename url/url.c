/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000 Free Software Foundation, Inc.

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

#include <url_mbox.h>
#include <url_unix.h>
#include <url_mdir.h>
#include <url_mmdf.h>
#include <url_pop.h>
#include <url_imap.h>
#include <url_mail.h>

#include <cpystr.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

/* Forward prototypes */
static int get_scheme  (const url_t, char *, size_t, size_t *);
static int get_mscheme (const url_t, char **, size_t *);
static int get_user    (const url_t, char *, size_t, size_t *);
static int get_muser   (const url_t, char **, size_t *);
static int get_passwd  (const url_t, char *, size_t, size_t *);
static int get_mpasswd (const url_t, char **, size_t *);
static int get_host    (const url_t, char *, size_t, size_t *);
static int get_mhost   (const url_t, char **, size_t *);
static int get_port    (const url_t, long *);
static int get_path    (const url_t, char *, size_t, size_t *);
static int get_mpath   (const url_t, char **, size_t *);
static int get_query   (const url_t, char *, size_t, size_t *);
static int get_mquery  (const url_t, char **, size_t *);
static int get_id      (const url_t, int *id);

/*
  Builtin url types.
  We are using a simple circular list to hold the builtin type. */
static struct url_builtin
{
  const struct url_type *utype;
  int is_malloc;
  struct url_builtin * next;
} url_builtin [] = {
  { NULL, 0,               &url_builtin[1] }, /* Sentinel, head list */
  { &_url_mbox_type, 0,    &url_builtin[2] },
  { &_url_unix_type, 0,    &url_builtin[3] },
  { &_url_maildir_type, 0, &url_builtin[4] },
  { &_url_mmdf_type, 0,    &url_builtin[5] },
  { &_url_pop_type, 0,     &url_builtin[6] },
  { &_url_imap_type, 0,    &url_builtin[7] },
  { &_url_mailto_type, 0,  &url_builtin[0] },
};

/*
  FIXME: Proper locking is not done when accessing the list
  this code is not thread-safe .. TODO */
int
url_add_type (struct url_type *utype)
{
  struct url_builtin *current = malloc (sizeof (*current));
  if (current == NULL)
    return -1;
  utype->id = (int)utype; /* It just has to be uniq */
  current->utype = utype;
  current->is_malloc = 1;
  current->next = url_builtin->next;
  url_builtin->next = current;
  return 0;
}

int
url_remove_type (const struct url_type *utype)
{
  struct url_builtin *current, *previous;
  for (previous = url_builtin, current = url_builtin->next;
       current != url_builtin;
       previous = current, current = current->next)
    {
      if (current->utype == utype)
	{
	  previous->next = current->next;
	  if (current->is_malloc)
	    free (current);
	  return 0;;
	}
    }
  return -1;
}

int
url_list_type (struct url_type *list, size_t len, size_t *n)
{
  struct url_builtin *current;
  size_t i = 0;
  for (i = 0, current = url_builtin->next; current != url_builtin;
       current = current->next, i++)
    {
      if (list)
	{
	  if (i < len)
	    list[i] = *(current->utype);
	  else
	    break;
	}
    }
  if (n)
    *n = i;
  return 0;
}

int
url_list_mtype (struct url_type **mlist, size_t *n)
{
  struct url_type *utype;
  size_t i;

  url_list_type (NULL, 0, &i);
  utype = calloc (i, sizeof (*utype));
  if (utype == NULL)
    {
      return ENOMEM;
    }

  *mlist = utype;
  url_list_type (utype, i, n);
  return 0;
}

int
url_init (url_t * purl, const char *name)
{
  int status = -1;
  const struct url_type *utype;
  struct url_builtin *ub;

  /* Sanity checks */
  if (name == NULL || *name == '\0')
    {
      return status;
    }

  /* Search for a known scheme */
  for (ub = url_builtin->next; ub != url_builtin; ub = ub->next)
    {
      utype = ub->utype;
      if (strncasecmp (name, utype->scheme, utype->len) == 0)
	{
	  status = 0;
	  break;
	}
    }

  /* Found one initialize it */
  if (status == 0)
    {
      status = utype->_init (purl, name);
      if (status == 0)
	{
	  url_t url = *purl;
	  if (url->utype == NULL)
	    url->utype = utype;
	  if (url->_get_scheme == NULL)
	    url->_get_scheme = get_scheme;
	  if (url->_get_mscheme == NULL)
	    url->_get_mscheme = get_mscheme;
	  if (url->_get_user == NULL)
	    url->_get_user = get_user;
	  if (url->_get_muser == NULL)
	    url->_get_muser = get_muser;
	  if (url->_get_passwd == NULL)
	    url->_get_passwd = get_passwd;
	  if (url->_get_mpasswd == NULL)
	    url->_get_mpasswd = get_mpasswd;
	  if (url->_get_host == NULL)
	    url->_get_host = get_host;
	  if (url->_get_mhost == NULL)
	    url->_get_mhost = get_mhost;
	  if (url->_get_port == NULL)
	    url->_get_port = get_port;
	  if (url->_get_path == NULL)
	    url->_get_path = get_path;
	  if (url->_get_mpath == NULL)
	    url->_get_mpath = get_mpath;
	  if (url->_get_query == NULL)
	    url->_get_query = get_query;
	  if (url->_get_mquery == NULL)
	    url->_get_mquery = get_mquery;
	  if (url->_get_id == NULL)
	    url->_get_id = get_id;
	}
    }
  return status;
}

void
url_destroy (url_t *purl)
{
  if (purl && *purl)
    {
      const struct url_type *utype = (*purl)->utype;
      utype->_destroy(purl);
      (*purl) = NULL;
    }
}


/* Simple stub functions they all call _cpystr */

static int
get_scheme (const url_t u, char *s, size_t len, size_t *n)
{
  size_t i;
  if (u == NULL)
    return EINVAL;
  i = _cpystr (s, u->scheme, len);
  if (n)
    *n = i;
  return 0;
}
static int
get_mscheme (const url_t u, char **s, size_t *n)
{
  size_t i;
  if (u == NULL || s == NULL || u->_get_scheme (u, NULL, 0, &i) != 0)
    {
      return EINVAL;
    }
  *s = malloc (++i);
  if (*s == NULL)
    {
      return ENOMEM;
    }
  u->_get_scheme (u, *s, 0, n);
  return 0;
}

static int
get_user (const url_t u, char *s, size_t len, size_t *n)
{
  size_t i;
  if (u == NULL)
    return EINVAL;
  i = _cpystr (s, u->user, len);
  if (n)
    *n = i;
  return 0;
}
static int
get_muser (const url_t u, char **s, size_t *n)
{
  size_t i;
  if (u == NULL || s == NULL || u->_get_user (u, NULL, 0, &i) != 0)
    {
      return EINVAL;
    }
  *s = malloc (++i);
  if (*s == NULL)
    {
      return ENOMEM;
    }
  u->_get_user (u, *s, i, n);
  return 0;
}

/* FIXME: We should not store passwd in clear, but rather
   have a simple encoding, and decoding mechanism */
static int
get_passwd (const url_t u, char *s, size_t len, size_t *n)
{
  size_t i;
  if (u == NULL)
    return EINVAL;
  i = _cpystr (s, u->passwd, len);
  if (n)
    *n = i;
  return 0;
}
static int
get_mpasswd (const url_t u, char **s, size_t *n)
{
  size_t i;
  if (u == NULL || s == NULL || u->_get_passwd (u, NULL, 0, &i) != 0)
    {
      return EINVAL;
    }
  *s = malloc (++i);
  if (*s == NULL)
    {
      return ENOMEM;
    }
  u->_get_passwd (u, *s, i, n);
  return 0;
}

static int
get_host (const url_t u, char *s, size_t len, size_t *n)
{
  size_t i;
  if (u == NULL)
    return EINVAL;
  i = _cpystr (s, u->host, len);
  if (n)
    *n = i;
  return 0;
}
static int
get_mhost (const url_t u, char **s, size_t *n)
{
  size_t i;
  if (u == NULL || s == NULL || u->_get_host (u, NULL, 0, &i) != 0)
    {
      return EINVAL;
    }
  *s = malloc (++i);
  if (*s == NULL)
    {
      return ENOMEM;
    }
  u->_get_host (u, *s, i, n);
  return 0;
}

static int
get_port (const url_t u, long * p)
{
  *p = u->port;
  return 0;
}

static int
get_path (const url_t u, char *s, size_t len, size_t *n)
{
  size_t i;
  if (u == NULL)
    return EINVAL;
  i = _cpystr(s, u->path, len);
  if (n)
    *n = i;
  return 0;
}
static int
get_mpath (const url_t u, char **s, size_t *n)
{
  size_t i;
  if (u == NULL || s == NULL || u->_get_path (u, NULL, 0, &i) != 0)
    {
      return EINVAL;
    }
  *s = malloc (++i);
  if (*s == NULL)
    {
      return ENOMEM;
    }
  u->_get_path (u, *s, i, n);
  return 0;
}

static int
get_query (const url_t u, char *s, size_t len, size_t *n)
{
  size_t i;
  if (u == NULL)
    return EINVAL;
  i = _cpystr(s, u->query, len);
  if (n)
    *n = i;
  return 0;
}
static int
get_mquery (const url_t u, char **s, size_t *n)
{
  size_t i;
  if (u == NULL || s == NULL || u->_get_query (u, NULL, 0, &i) != 0)
    {
      return EINVAL;
    }
  *s = malloc (++i);
  if (*s == NULL)
    {
      return ENOMEM;
    }
  u->_get_query (u, *s, i, n);
  return 0;
}

static int
get_id (const url_t u, int *id)
{
  const struct url_type *utype = u->utype;
  *id = utype->id;
  return 0;
}
