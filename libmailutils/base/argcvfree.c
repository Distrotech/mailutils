/* Free an array of string pointers.
   Copyright (C) 1999-2001, 2003-2006, 2010-2012 Free Software
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <stdlib.h>
#include <mailutils/types.h>
#include <mailutils/argcv.h>

/*
 * frees all elements of an argv array
 * argc is the number of elements
 * argv is the array
 */
void
mu_argcv_free (size_t argc, char **argv)
{
  size_t i;

  for (i = 0; i < argc; i++)
    if (argv[i])
      free (argv[i]);
  free (argv);
}

void
mu_argv_free (char **argv)
{
  int i;

  for (i = 0; argv[i]; i++)
    free (argv[i]);
  free (argv);
}

