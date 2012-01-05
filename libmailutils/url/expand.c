/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2010-2012 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library.  If not, see 
   <http://www.gnu.org/licenses/>. */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif

#include <mailutils/types.h>
#include <mailutils/util.h>
#include <mailutils/errno.h>
#include <mailutils/argcv.h>
#include <mailutils/sys/url.h>

/* Default mailbox path generator */
static char *
_url_path_default (const char *spooldir, const char *user, int unused)
{
  return mu_make_file_name (spooldir, user);
}

/* Hashed indexing */
static char *
_url_path_hashed (const char *spooldir, const char *user, int param)
{
  int i;
  int ulen = strlen (user);
  char *mbox;
  unsigned hash;

  if (param > ulen)
    param = ulen;
  for (i = 0, hash = 0; i < param; i++)
    hash += user[i];

  mbox = malloc (ulen + strlen (spooldir) + 5);
  sprintf (mbox, "%s/%02X/%s", spooldir, hash % 256, user);
  return mbox;
}

static int transtab[] = {
  'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h',
  'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p',
  'q', 'r', 's', 't', 'u', 'v', 'w', 'x',
  'y', 'z', 'a', 'b', 'c', 'd', 'e', 'f',
  'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
  'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
  'w', 'x', 'y', 'z', 'a', 'b', 'c', 'd',
  'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l',
  'm', 'a', 'b', 'c', 'd', 'e', 'f', 'g',
  'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
  'p', 'q', 'r', 's', 't', 'u', 'v', 'w',
  'x', 'y', 'z', 'b', 'c', 'd', 'e', 'f',
  'g', 'a', 'b', 'c', 'd', 'e', 'f', 'g',
  'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
  'p', 'q', 'r', 's', 't', 'u', 'v', 'w',
  'x', 'y', 'z', 'b', 'c', 'd', 'e', 'f',
  'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i',
  'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q',
  'r', 's', 't', 'u', 'v', 'w', 'x', 'y',
  'z', 'a', 'b', 'c', 'd', 'e', 'f', 'g',
  'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
  'p', 'q', 'r', 's', 't', 'u', 'v', 'w',
  'x', 'y', 'z', 'a', 'b', 'c', 'd', 'e',
  'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
  'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h',
  'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p',
  'q', 'r', 's', 't', 'u', 'v', 'w', 'x',
  'y', 'z', 'b', 'c', 'd', 'e', 'f', 'g',
  'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h',
  'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p',
  'q', 'r', 's', 't', 'u', 'v', 'w', 'x',
  'y', 'z', 'b', 'c', 'd', 'e', 'f', 'g'
};

/* Forward Indexing */
static char *
_url_path_index (const char *spooldir, const char *iuser, int index_depth)
{
  const unsigned char* user = (const unsigned char*) iuser;
  int i, ulen = strlen (iuser);
  char *mbox, *p;

  if (ulen == 0)
    return NULL;

  mbox = malloc (ulen + strlen (spooldir) + 2*index_depth + 2);
  strcpy (mbox, spooldir);
  p = mbox + strlen (mbox);
  for (i = 0; i < index_depth && i < ulen; i++)
    {
      *p++ = '/';
      *p++ = transtab[ user[i] ];
    }
  for (; i < index_depth; i++)
    {
      *p++ = '/';
      *p++ = transtab[ user[ulen-1] ];
    }
  *p++ = '/';
  strcpy (p, iuser);
  return mbox;
}

/* Reverse Indexing */
static char *
_url_path_rev_index (const char *spooldir, const char *iuser, int index_depth)
{
  const unsigned char* user = (const unsigned char*) iuser;
  int i, ulen = strlen (iuser);
  char *mbox, *p;

  if (ulen == 0)
    return NULL;

  mbox = malloc (ulen + strlen (spooldir) + 2*index_depth + 1);
  strcpy (mbox, spooldir);
  p = mbox + strlen (mbox);
  for (i = 0; i < index_depth && i < ulen; i++)
    {
      *p++ = '/';
      *p++ = transtab[ user[ulen - i - 1] ];
    }
  for (; i < index_depth; i++)
    {
      *p++ = '/';
      *p++ = transtab[ user[0] ];
    }
  *p++ = '/';
  strcpy (p, iuser);
  return mbox;
}

static int
rmselector (const char *p, void *data MU_ARG_UNUSED)
{
  return strncmp (p, "type=", 5) == 0
	 || strncmp (p, "user=", 5) == 0
	 || strncmp (p, "param=", 6) == 0;
}

int
mu_url_expand_path (mu_url_t url)
{
  size_t i;
  char *user = NULL;
  int param = 0;
  char *p;
  char *(*fun) (const char *, const char *, int) = _url_path_default;

  if (url->fvcount == 0)
    return 0;

  for (i = 0; i < url->fvcount; i++)
    {
      p = url->fvpairs[i];
      if (strncmp (p, "type=", 5) == 0)
	{
	  char *type = p + 5;

	  if (strcmp (type, "hash") == 0)
	    fun = _url_path_hashed;
	  else if (strcmp (type, "index") == 0)
	    fun = _url_path_index;
	  else if (strcmp (type, "rev-index") == 0)
	    fun = _url_path_rev_index;
	  else
	    return MU_ERR_NOENT;
	}
      else if (strncmp (p, "user=", 5) == 0)
	{
	  user = p + 5;
	}
      else if (strncmp (p, "param=", 6) == 0)
	{
	  param = strtoul (p + 6, NULL, 0);
	}
    }

  if (user)
    {
      char *p = fun (url->path, user, param);
      if (p)
	{
	  free (url->path);
	  url->path = p;
	}
      mu_argcv_remove (&url->fvcount, &url->fvpairs, rmselector, NULL);
    }
  else
    return MU_ERR_NOENT;

  return 0;
}

