/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2002, 2003 Free Software Foundation, Inc.

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

#ifdef ENABLE_MH

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <url0.h>
#include <registrar0.h>

static void
url_mh_destroy (url_t url)
{
  (void) url;
}

/*
  MH url
  mh:path
*/
int
_url_mh_init (url_t url)
{
  const char *name = url_to_string (url);
  size_t len = strlen (name);

  /* reject the obvious */
  if (name == NULL || strncmp (MU_MH_SCHEME, name, MU_MH_SCHEME_LEN) != 0
      || len < (MU_MH_SCHEME_LEN + 1) /* (scheme)+1(path)*/)
    return EINVAL;

  /* do I need to decode url encoding '% hex hex' ? */

  /* TYPE */
  url->_destroy = url_mh_destroy;

  /* SCHEME */
  url->scheme = strdup (MU_MH_SCHEME);
  if (url->scheme == NULL)
    {
      url_mh_destroy (url);
      return ENOMEM;
    }

  /* PATH */
  name += MU_MH_SCHEME_LEN; /* pass the scheme */
  url->path = strdup (name);
  if (url->path == NULL)
    {
      url_mh_destroy (url);
      return ENOMEM;
    }

  return 0;
}

#endif
