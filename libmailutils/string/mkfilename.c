/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2009-2012 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library.  If not,
   see <http://www.gnu.org/licenses/>. */

#if HAVE_CONFIG_H
# include <config.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mailutils/alloc.h>
#include <mailutils/util.h>

char *
mu_make_file_name_suf (const char *dir, const char *file, const char *suf)
{
  char *tmp;
  size_t dirlen = strlen (dir);
  size_t suflen = suf ? strlen (suf) : 0;
  size_t fillen = strlen (file);
  size_t len;

  while (dirlen > 0 && dir[dirlen-1] == '/')
    dirlen--;
  
  len = dirlen + (dir[0] ? 1 : 0) + fillen + suflen;
  tmp = mu_alloc (len + 1);
  if (tmp)
    {
      memcpy (tmp, dir, dirlen);
      if (dir[0])
	tmp[dirlen++] = '/';
      memcpy (tmp + dirlen, file, fillen);
      if (suf)
	memcpy (tmp + dirlen + fillen, suf, suflen);
      tmp[len] = 0;
    }
  return tmp;
}
