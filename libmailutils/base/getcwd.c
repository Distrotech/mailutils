/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2003, 2005-2007, 2009-2012 Free Software
   Foundation, Inc.

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

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

char *
mu_getcwd ()
{
  char *ret;
  unsigned path_max;
  char buf[128];

  errno = 0;
  ret = getcwd (buf, sizeof (buf));
  if (ret != NULL)
    return strdup (buf);

  if (errno != ERANGE)
    return NULL;

  path_max = 128;
  path_max += 2;                /* The getcwd docs say to do this. */

  for (;;)
    {
      char *cwd = (char *) malloc (path_max);

      errno = 0;
      ret = getcwd (cwd, path_max);
      if (ret != NULL)
        return ret;
      if (errno != ERANGE)
        {
          int save_errno = errno;
          free (cwd);
          errno = save_errno;
          return NULL;
        }

      free (cwd);

      path_max += path_max / 16;
      path_max += 32;
    }
  /* oops?  */
  return NULL;
}

