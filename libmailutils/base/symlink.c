/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002, 2003, 2005, 2006, 2007, 2009,
   2010 Free Software Foundation, Inc.

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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <mailutils/types.h>
#include <mailutils/util.h>

/* The result of readlink() may be a path relative to that link, 
 * qualify it if necessary.
 */
static void
mu_qualify_link (const char *path, const char *link, char *qualified)
{
  const char *lb = NULL;
  size_t len;

  /* link is full path */
  if (*link == '/')
    {
      mu_cpystr (qualified, link, _POSIX_PATH_MAX);
      return;
    }

  if ((lb = strrchr (path, '/')) == NULL)
    {
      /* no path in link */
      mu_cpystr (qualified, link, _POSIX_PATH_MAX);
      return;
    }

  len = lb - path + 1;
  memcpy (qualified, path, len);
  mu_cpystr (qualified + len, link, _POSIX_PATH_MAX - len);
}

#ifndef _POSIX_SYMLOOP_MAX
# define _POSIX_SYMLOOP_MAX 255
#endif

int
mu_unroll_symlink (char *out, size_t outsz, const char *in)
{
  char path[_POSIX_PATH_MAX];
  int symloops = 0;

  while (symloops++ < _POSIX_SYMLOOP_MAX)
    {
      struct stat s;
      char link[_POSIX_PATH_MAX];
      char qualified[_POSIX_PATH_MAX];
      int len;

      if (lstat (in, &s) == -1)
	return errno;

      if (!S_ISLNK (s.st_mode))
	{
	  mu_cpystr (path, in, sizeof (path));
	  break;
	}

      if ((len = readlink (in, link, sizeof (link))) == -1)
	return errno;

      link[(len >= sizeof (link)) ? (sizeof (link) - 1) : len] = '\0';

      mu_qualify_link (in, link, qualified);

      mu_cpystr (path, qualified, sizeof (path));

      in = path;
    }

  mu_cpystr (out, path, outsz);

  return 0;
}

