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

#include <url0.h>
#include <registrar.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>

static void url_mmdf_destroy (url_t *purl);
static int url_mmdf_init (url_t *purl, const char *name);

struct url_registrar  _url_mmdf_registrar =
{
  "mmdf:",
  url_mmdf_init, url_mmdf_destroy
};

static void
url_mmdf_destroy (url_t *purl)
{
  if (purl && *purl)
    {
      url_t url = *purl;
      free (url->scheme);
      free (url->path);
      free (url);
      *purl = NULL;
    }
}

/*
  MMDF URL
  mmdf:
*/
static int
url_mmdf_init (url_t *purl, const char *name)
{
  url_t url;
  struct url_registrar *ureg = &_url_mmdf_registrar;
  size_t len, scheme_len = strlen (ureg->scheme);

  /* reject the obvious */
  if (name == NULL || strncmp (ureg->scheme, name, scheme_len) != 0
      || (len = strlen (name)) < 8 /* 7(scheme)+1(path)*/)
    return EINVAL;

  /* do I need to decode url encoding '% hex hex' ? */

  url = calloc(1, sizeof (*url));
  if (url == NULL)
    return ENOMEM;

  /* TYPE */
  url->_init = ureg->_init;
  url->_destroy = ureg->_destroy;

  /* SCHEME */
  url->scheme = strdup (ureg->scheme);
  if (url->scheme == NULL)
    {
      ureg->_destroy (&url);
      return ENOMEM;
    }

  /* PATH */
  name += scheme_len; /* pass the scheme */
  url->path = strdup (name);
  if (url->path == NULL)
    {
      ureg->_destroy (&url);
      return ENOMEM;
    }

  *purl = url;
  return 0;
}
