/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2004 Free Software Foundation, Inc.

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

extern int iterator_create   __P ((iterator_t *, void *));
extern int iterator_dup      __P ((iterator_t *piterator, iterator_t orig));
extern void iterator_destroy __P ((iterator_t *));
extern int iterator_first    __P ((iterator_t));
extern int iterator_next     __P ((iterator_t));
extern int iterator_current  __P ((iterator_t, void * const *pitem));
extern int iterator_is_done  __P ((iterator_t));

extern int iterator_attach (iterator_t *root, iterator_t iterator);
extern int iterator_detach (iterator_t *root, iterator_t iterator);
extern void iterator_advance (iterator_t iterator, void *e);
  
extern int iterator_set_first (iterator_t, int (*first) (void *));  
extern int iterator_set_next (iterator_t, int (*next) (void *));  
extern int iterator_set_getitem (iterator_t,
				 int (*getitem) (void *, void **));  
extern int iterator_set_finished_p (iterator_t,
				    int (*finished_p) (void *));  
extern int iterator_set_dup (iterator_t itr,
			     int (dup) (void **ptr, void *data));
extern int iterator_set_destroy (iterator_t itr,
				 int (destroy) (iterator_t, void *data));
extern int iterator_set_curitem_p (iterator_t itr,
				   int (*curitem_p) (void *, void *));
  
#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_ITERATOR_H */
