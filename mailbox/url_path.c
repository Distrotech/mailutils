/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000 Free Software Foundation, Inc.

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

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <registrar0.h>
#include <url0.h>

static void url_path_destroy (url_t);

static void
url_path_destroy (url_t url ARG_UNUSED)
{
}

int
_url_path_init (url_t url)
{
  const char *name = url_to_string (url);
  /* reject the obvious */
  if (name == NULL || *name == '\0')
    return EINVAL;

  /* FIXME: do I need to decode url encoding '% hex hex' ? */

  /* TYPE */
  url->_destroy = url_path_destroy;

  /* SCHEME */
  url->scheme = strdup (MU_PATH_SCHEME);
  if (url->scheme == NULL)
    {
      url_path_destroy (url);
      return ENOMEM;
    }

  /* PATH */
  url->path = strdup (name);
  if (url->path == NULL)
    {
      url_path_destroy (url);
      return ENOMEM;
    }

  return 0;
}
