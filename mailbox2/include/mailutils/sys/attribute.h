/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

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

#ifndef _MAILUTILS_SYS_ATTRIBUTE_H
# define _MAILUTILS_SYS_ATTRIBUTE_H

#ifdef DMALLOC
#  include <dmalloc.h>
#endif

#include <mailutils/attribute.h>
#include <mailutils/monitor.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __P
# ifdef __STDC__
#  define __P(args) args
# else
#  define __P(args) ()
# endif
#endif /*__P */

struct _attribute_vtable
{
  int (*add_ref)   __P ((attribute_t));
  int (*release)   __P ((attribute_t));
  int (*destroy)   __P ((attribute_t));

  int (*get_flags)   __P ((attribute_t, int *));
  int (*set_flags)   __P ((attribute_t, int));
  int (*unset_flags) __P ((attribute_t, int));
  int (*clear_flags) __P ((attribute_t));
};

struct _attribute
{
  struct _attribute_vtable *vtable;
};

/* A simple default attribute implementation.  */
struct _da
{
  struct _attribute base;
  int ref;
  int flags;
  monitor_t lock;
};

#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_SYS_ATTRIBUTE_H */
