/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999 Free Software Foundation, Inc.

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

#include <url_mmdf.h>
#include <stdlib.h>
#include <string.h>

struct url_type  _url_mmdf_type =
{
  "mmdf://", 7,
  "mmdf://<full_path_name>",
  (int)&_url_mmdf_type, /* uniq id */
  url_mmdf_init, url_mmdf_destroy
};

void
url_mmdf_destroy (url_t *purl)
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
  MMDF URL
  mmdf://path
*/
int
url_mmdf_init (url_t *purl, const char *name)
{
  url_t url;
  int len;
  struct url_type *utype = &_url_mmdf_type;

  /* reject the obvious */
  if (name == NULL || strncmp (utype->scheme, name,
			       utype->len) != 0
      || (len = strlen (name)) < 8 /* 7(scheme)+1(path)*/)
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
  /* strdup ("mmdf://") the hard way */
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
  memcpy (url->path, name, len + 1 /* including the NULL */);

  *purl = url;
  return 0;
}
