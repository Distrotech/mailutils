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

#ifndef _MAILUTILS_ITERATOR_H
#define _MAILUTILS_ITERATOR_H

#include <mailutils/mu_features.h>
#include <mailutils/list.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _iterator;
typedef struct _iterator *iterator_t;

extern int iterator_create   __P ((iterator_t *, list_t));
extern void iterator_destroy __P ((iterator_t *));
extern int iterator_first    __P ((iterator_t));
extern int iterator_next     __P ((iterator_t));
extern int iterator_current  __P ((iterator_t, void **pitem));
extern int iterator_is_done  __P ((iterator_t));

#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_ITERATOR_H */
