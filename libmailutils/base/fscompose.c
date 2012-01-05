/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2011-2012 Free Software Foundation, Inc.

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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <string.h>
#include <mailutils/types.h>
#include <mailutils/util.h>
#include <mailutils/nls.h>
#include <mailutils/error.h>
#include <mailutils/errno.h>

int
mu_file_safety_compose (int *res, const char *name, int defval)
{
  int negate = 0;
  int val;
  
  if (*name == '-')
    {
      negate = 1;
      name++;
    }
  else if (*name == '+')
    name++;

  if (strcmp (name, "none") == 0)
    {
      *res = negate ? MU_FILE_SAFETY_ALL : MU_FILE_SAFETY_NONE;
      return 0;
    }
  else if (strcmp (name, "all") == 0)
    {
      *res = negate ? MU_FILE_SAFETY_NONE : MU_FILE_SAFETY_ALL;
      return 0;
    }
  else if (strcmp (name, "default") == 0)
    val = defval;
  else if (mu_file_safety_name_to_code (name, &val))
    return MU_ERR_NOENT;

  if (negate)
    *res &= ~val;
  else
    *res |= val;
  
  return 0;
}
