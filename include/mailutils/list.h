/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2005 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library; if not, write to the
   Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301 USA */

#ifndef _MAILUTILS_LIST_H
#define _MAILUTILS_LIST_H

#include <mailutils/types.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int mu_list_create   (list_t *);
extern void mu_list_destroy (list_t *);
extern int mu_list_append   (list_t, void *item);
extern int mu_list_prepend  (list_t, void *item);
extern int mu_list_insert   (list_t list, void *item, void *new_item, 
                          int insert_before);
extern int mu_list_is_empty (list_t);
extern int mu_list_count    (list_t, size_t *pcount);
extern int mu_list_remove   (list_t, void *item);
extern int mu_list_replace  (list_t list, void *old_item, void *new_item);  
extern int mu_list_get      (list_t, size_t _index, void **pitem);
extern int mu_list_to_array (list_t list, void **array, size_t count, size_t *pcount);
extern int mu_list_locate   (list_t list, void *item, void **ret_item);
extern int mu_list_get_iterator (list_t, iterator_t *);

typedef int mu_list_action_t (void* item, void* cbdata);
  
extern int mu_list_do       (list_t list, mu_list_action_t * action, void *cbdata);

typedef int (*mu_list_comparator_t) (const void*, const void*);

extern mu_list_comparator_t mu_list_set_comparator (list_t, mu_list_comparator_t);

extern int mu_list_set_destroy_item (list_t list, void (*destoy_item) (void *));

  
#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_LIST_H */
