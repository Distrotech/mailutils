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

#ifndef _ATTRIBUTE0_H
# define _ATTRIBUTE0_H

#ifdef DMALLOC
#  include <dmalloc.h>
#endif

#include <mailutils/attribute.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _attribute
{
  void *owner;

  int flags;
  int user_flags;

  int (*_get_flags)   __P ((attribute_t, int *));
  int (*_set_flags)   __P ((attribute_t, int));
  int (*_unset_flags) __P ((attribute_t, int));
};

#ifdef __cplusplus
}
#endif

#endif /* _ATTRIBUTE0_H */
