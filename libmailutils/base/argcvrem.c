/* Selectively remove elements from an array of string pointers.
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

void
mu_argcv_remove (int *pargc, char ***pargv,
		 int (*sel) (const char *, void *), void *data)
{
  int i, j;
  int argc = *pargc;
  char **argv = *pargv;
  int cnt = 0;
  
  for (i = j = 0; i < argc; i++)
    {
      if (sel (argv[i], data))
	{
	  free (argv[i]);
	  cnt++;
	}
      else
	{
	  if (i != j)
	    argv[j] = argv[i];
	  j++;
	}
    }
  if (i != j)
    argv[j] = NULL;
  argc -= cnt;

  *pargc = argc;
  *pargv = argv;
}
