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

#include <url_mbox.h>
#include <stdlib.h>
#include <string.h>

struct url_type  _url_mbox_type =
{
  "file://", 7,
  "file://<full_path_name>",
  (int)&_url_mbox_type, /* uniq id */
  url_mbox_init, url_mbox_destroy
};

void
url_mbox_destroy (url_t *purl)
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
  UNIXMBOX and MAILDIR URL
  file://path
*/
int
url_mbox_init (url_t *purl, const char *name)
{
  url_t url;
  int len;

  /* reject the obvious */
  if (name == NULL || strncmp ("file://", name, 7) != 0
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
  url->utype = &_url_mbox_type;

  /* SCHEME */
  /* strdup ("file://") the hard way */
  url->scheme = malloc (7 + 1);
  if (url->scheme == NULL)
    {
      url_mbox_destroy (&url);
      return -1;
    }
  memcpy (url->scheme, "file://", 8);

  /* PATH */
  name += 7; /* pass the scheme */
  len -= 7;
  url->path = malloc (len + 1);
  if (url->path == NULL)
    {
      url_mbox_destroy (&url);
      return -1;
    }
  memcpy (url->path, name, len + 1);

  *purl = url;
  return 0;
}
