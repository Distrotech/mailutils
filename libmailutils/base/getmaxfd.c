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

#include <unistd.h>
#include <limits.h>

#if defined (HAVE_SYSCONF) && defined (_SC_OPEN_MAX)
# define __getmaxfd() sysconf (_SC_OPEN_MAX)
#elif defined (HAVE_GETDTABLESIZE)
# define __getmaxfd() getdtablesize ()
#elif defined OPEN_MAX
# define __getmaxfd() OPEN_MAX
#else
# define __getmaxfd() 256
#endif

int
mu_getmaxfd ()
{
  return __getmaxfd ();
}

