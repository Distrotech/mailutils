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

#ifndef _MAILUTILS_LIST_H
#define _MAILUTILS_LIST_H

#include <mailutils/types.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int list_create   __P ((list_t *));
extern void list_destroy __P ((list_t *));
extern int list_append   __P ((list_t, void *item));
extern int list_prepend  __P ((list_t, void *item));
extern int list_is_empty __P ((list_t));
extern int list_count    __P ((list_t, size_t *pcount));
extern int list_remove   __P ((list_t, void *item));
extern int list_replace  __P ((list_t list, void *old_item, void *new_item));  
extern int list_get      __P ((list_t, size_t _index, void **pitem));

typedef int list_action_t __PMT ((void* item, void* cbdata));
  
extern int list_do       __P ((list_t list, list_action_t * action, void *cbdata));

typedef int (*list_comparator_t) __PMT((const void*, const void*));

extern list_comparator_t list_set_comparator __P((list_t, list_comparator_t));
  
#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_LIST_H */
