/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

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

#ifndef _MAILUTILS_SYS_DEBUG_H
#define _MAILUTILS_SYS_DEBUG_H

#ifdef DMALLOC
#include <dmalloc.h>
#endif

#include <mailutils/debug.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _mu_debug_vtable
{
  int  (*ref)       __P ((mu_debug_t));
  void (*destroy)   __P ((mu_debug_t *));

  int  (*get_level) __P ((mu_debug_t, unsigned int *));
  int  (*set_level) __P ((mu_debug_t, unsigned int));
  int  (*print)     __P ((mu_debug_t, unsigned int, const char *));
};

struct _mu_debug
{
  struct _mu_debug_vtable *vtable;
};

#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_SYS_DEBUG_H */
