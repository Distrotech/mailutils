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

#ifndef _MAILUTILS_SYS_DATTRIBUTE_H
# define _MAILUTILS_SYS_DATTRIBUTE_H

#ifdef DMALLOC
#  include <dmalloc.h>
#endif

#include <mailutils/sys/attribute.h>
#include <mailutils/refcount.h>

#ifdef __cplusplus
extern "C" {
#endif

/* A simple default attribute implementation.  */
struct _attribute_default
{
  struct _attribute base;
  mu_refcount_t refcount;
  int flags;
  int userflags;
};

extern int  _attribute_default_ctor        __P ((struct _attribute_default *));
extern void _attribute_default_dtor        __P ((attribute_t));
extern int  _attribute_default_ref         __P ((attribute_t));
extern void _attribute_default_destroy     __P ((attribute_t *));

extern int  _attribute_default_get_flags   __P ((attribute_t, int *));
extern int  _attribute_default_set_flags   __P ((attribute_t, int));
extern int  _attribute_default_unset_flags __P ((attribute_t, int));
extern int  _attribute_default_clear_flags __P ((attribute_t));

extern int  _attribute_default_get_userflags   __P ((attribute_t, int *));
extern int  _attribute_default_set_userflags   __P ((attribute_t, int));
extern int  _attribute_default_unset_userflags __P ((attribute_t, int));
extern int  _attribute_default_clear_userflags __P ((attribute_t));

#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_SYS_ATTRIBUTE_H */
