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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <registrar0.h>
#include <url0.h>

static void url_mbox_destroy (url_t purl);

static void
url_mbox_destroy (url_t url)
{
  (void) url;
}

/*
  UNIX Mbox
  mbox:path
*/
int
_url_mbox_init (url_t url)
{
  const char *name = url_to_string (url);
  size_t len = strlen (name);

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

  return 0;
}
