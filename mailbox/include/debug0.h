/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Library Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifndef _MAILUTILS_DEBUG0_H
#define _MAILUTILS_DEBUG0_H

#ifdef DMALLOC
#  include <dmalloc.h>
#endif

#ifndef __P
#ifdef __STDC__
#define __P(args) args
#else
#define __P(args) ()
#endif
#endif /*__P */

#include <mailutils/debug.h>

#ifdef _cplusplus
extern "C" {
#endif

struct _debug
{
  size_t level;
  char *buffer;
  size_t buflen;
  void *owner;
  int (*_print) __P ((debug_t, const char *, va_list));
};

#ifdef _cplusplus
}
#endif

#endif /* _MAILUTILS_DEBUG0_H */
