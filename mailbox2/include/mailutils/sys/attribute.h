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

#ifndef _MAILUTILS_SYS_ATTRIBUTE_H
# define _MAILUTILS_SYS_ATTRIBUTE_H

#ifdef DMALLOC
# include <dmalloc.h>
#endif

#include <mailutils/attribute.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _attribute_vtable
{
  int  (*ref)       __P ((attribute_t));
  void (*destroy)   __P ((attribute_t *));

  int  (*get_flags)   __P ((attribute_t, int *));
  int  (*set_flags)   __P ((attribute_t, int));
  int  (*unset_flags) __P ((attribute_t, int));
  int  (*clear_flags) __P ((attribute_t));

  int  (*get_userflags)   __P ((attribute_t, int *));
  int  (*set_userflags)   __P ((attribute_t, int));
  int  (*unset_userflags) __P ((attribute_t, int));
  int  (*clear_userflags) __P ((attribute_t));
};

struct _attribute
{
  struct _attribute_vtable *vtable;
};

#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_SYS_ATTRIBUTE_H */
