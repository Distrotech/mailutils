/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU Library General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with GNU Mailutils; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

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

#include <registrar0.h>
#include <url0.h>

static void url_mbox_destroy (url_t purl);

static void
url_mbox_destroy (url_t url)
{
  (void) url;
}

/* Default mailbox path generator */
static char *
_url_path_default (const char *spooldir, const char *user, int unused)
{
  char *mbox = malloc (sizeof(spooldir) + strlen(user) + 2);
  if (!mbox)
    errno = ENOMEM;
  else
    sprintf (mbox, "%s/%s", spooldir, user);
  return mbox;
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
  int i, ulen = strlen (user);
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
  strcpy (p, user);
  return mbox;
}

/* Reverse Indexing */
static char *
_url_path_rev_index (const char *spooldir, const char *iuser, int index_depth)
{
  const unsigned char* user = (const unsigned char*) iuser;
  int i, ulen = strlen (user);
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
  strcpy (p, user);
  return mbox;
}

/*
  UNIX Mbox
  mbox:path[;type=TYPE][;param=PARAM][;user=USERNAME]
*/
int
_url_mbox_init (url_t url)
{
  const char *name = url_to_string (url);
  size_t len = strlen (name);
  char *p;
  
  /* reject the obvious */
  if (name == NULL || strncmp (MU_MBOX_SCHEME, name, MU_MBOX_SCHEME_LEN) != 0
      || len < (MU_MBOX_SCHEME_LEN + 1) /* (scheme)+1(path)*/)
    return EINVAL;

  /* do I need to decode url encoding '% hex hex' ? */

  /* TYPE */
  url->_destroy = url_mbox_destroy;

  /* SCHEME */
  url->scheme = strdup (MU_MBOX_SCHEME);
  if (url->scheme == NULL)
    {
      url_mbox_destroy (url);
      return ENOMEM;
    }

  /* PATH */
  name += MU_MBOX_SCHEME_LEN; /* pass the scheme */
  url->path = strdup (name);
  if (url->path == NULL)
    {
      url_mbox_destroy (url);
      return ENOMEM;
    }
  p = strchr (url->path, ';');
  if (p)
    {
      char *(*fun)() = _url_path_default;
      char *user = NULL;
      int param = 0;
      
      *p++ = 0;
      while (p)
	{
	  char *q = strchr (p, ';');
	  if (q)
	    *q++ = 0;
	  if (strncasecmp (p, "type=", 5) == 0)
	    {
	      char *type = p + 5;

	      if (strcmp (type, "hash") == 0)
		fun = _url_path_hashed;
	      else if (strcmp (type, "index") == 0)
		fun = _url_path_index;
	      else if (strcmp (type, "rev-index") == 0)
		fun = _url_path_rev_index;
	      else
		{
		  url_mbox_destroy (url);
		  return ENOENT;
		}
	    }
	  else if (strncasecmp (p, "user=", 5) == 0)
	    {
	      user = p + 5;
	    }
	  else if (strncasecmp (p, "param=", 6) == 0)
	    {
	      param = strtoul (p+6, NULL, 0);
	    }
	  p = q;
	}

      if (user)
	{
	  p = fun (url->path, user, param);
	  free (url->path);
	  url->path = p;
	}
      else
	{
	  url_mbox_destroy (url);
	  return ENOENT;
	}
    }

  return 0;
}

