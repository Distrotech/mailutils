/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000 Free Software Foundation, Inc.

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

#ifndef _MAILUTILS_ITERATOR_H
#define _MAILUTILS_ITERATOR_H

#include <mailutils/types.h>

#ifdef __cplusplus
extern "C" {
#endif

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
