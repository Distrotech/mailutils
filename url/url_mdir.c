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

#include <url_mdir.h>
#include <stdlib.h>
#include <string.h>

struct url_type  _url_maildir_type =
{
  "maildir://", 10,
  "maildir://<full_directory_name>",
  (int)&_url_maildir_type, /* uniq id */
  url_maildir_init, url_maildir_destroy
};

void
url_maildir_destroy (url_t *purl)
{
  if (purl && *purl)
    {
      url_t url = *purl;
      if (url->scheme)
	free (url->scheme);
      if (url->path)
	free (url->path);
      free (url);
      url = NULL;
    }
}

/*
  MAILDIR URL
  maildir://path
*/
int
url_maildir_init (url_t *purl, const char *name)
{
  url_t url;
  size_t len;
  struct url_type *utype = &_url_maildir_type;

  /* reject the obvious */
  if (name == NULL || strncmp (utype->scheme, name, utype->len) != 0
      || (len = strlen (name)) < (utype->len + 1) /* (scheme)+1(path)*/)
    {
      return -1;
    }

  /* do I need to decode url encoding '% hex hex' ? */

  url = calloc(1, sizeof (*url));
  if (url == NULL)
    {
      return -1;
    }

  /* TYPE */
  url->utype = utype;

  /* SCHEME */
  /* strdup ("maildir://") the hard way */
  url->scheme = malloc (utype->len + 1);
  if (url->scheme == NULL)
    {
      utype->_destroy (&url);
      return -1;
    }
  memcpy (url->scheme, utype->scheme, utype->len + 1 /* including the NULL */);

  /* PATH */
  name += utype->len; /* pass the scheme */
  len -= utype->len; /* decremente the len */
  url->path = malloc (len + 1);
  if (url->path == NULL)
    {
      utype->_destroy (&url);
      return -1;
    }
  memcpy (url->path, name, len + 1 /* including the NULL*/);

  *purl = url;
  return 0;
}
