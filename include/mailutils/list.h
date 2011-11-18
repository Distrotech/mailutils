/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2005, 2007, 2008, 2010, 2011 Free Software
   Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library.  If not, see 
   <http://www.gnu.org/licenses/>. */

#ifndef _MAILUTILS_LIST_H
#define _MAILUTILS_LIST_H

#include <mailutils/types.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int mu_list_create   (mu_list_t *);
extern void mu_list_destroy (mu_list_t *);
extern void mu_list_clear (mu_list_t list);
extern int mu_list_append   (mu_list_t, void *item);
extern int mu_list_prepend  (mu_list_t, void *item);
extern int mu_list_insert   (mu_list_t list, void *item, void *new_item, 
                          int insert_before);
extern int mu_list_is_empty (mu_list_t);
extern int mu_list_count    (mu_list_t, size_t *pcount);
extern int mu_list_remove   (mu_list_t, void *item);
extern int mu_list_remove_nd  (mu_list_t, void *item);
extern int mu_list_replace  (mu_list_t list, void *old_item, void *new_item);  
extern int mu_list_replace_nd (mu_list_t list, void *old_item, void *new_item);  
extern int mu_list_get      (mu_list_t, size_t _index, void **pitem);
extern int mu_list_to_array (mu_list_t list, void **array, size_t count, size_t *pcount);
extern int mu_list_locate   (mu_list_t list, void *item, void **ret_item);
extern int mu_list_get_iterator (mu_list_t, mu_iterator_t *);

typedef int mu_list_action_t (void *item, void *cbdata);

extern int mu_list_do       (mu_list_t list, mu_list_action_t *action, void *cbdata);

typedef int (*mu_list_comparator_t) (const void*, const void*);

extern int _mu_list_ptr_comparator (const void*, const void*);
  
extern mu_list_comparator_t mu_list_set_comparator (mu_list_t,
						    mu_list_comparator_t);
extern int mu_list_get_comparator (mu_list_t, mu_list_comparator_t *);

extern void mu_list_free_item (void *item);

typedef void (*mu_list_destroy_item_t) (void *);
  
extern mu_list_destroy_item_t mu_list_set_destroy_item
              (mu_list_t list, mu_list_destroy_item_t destroy_item);


extern int mu_list_intersect_dup (mu_list_t *, mu_list_t, mu_list_t,
				  int (*dup_item) (void **, void *, void *),
				  void *);
extern int mu_list_intersect (mu_list_t *, mu_list_t, mu_list_t);  

extern int mu_list_insert_list (mu_list_t list, void *item, mu_list_t new_list,
				int insert_before);
extern void mu_list_append_list (mu_list_t list, mu_list_t new_list);
extern void mu_list_prepend_list (mu_list_t list, mu_list_t new_list);

/* List mapper functions */
typedef int (*mu_list_mapper_t) (void **itmv, size_t itmc, void *call_data);

/* A generalized list mapping function.

   Mu_list_gmap iterates over the LIST, gathering its elements in an
   array of type void **.  When NELEM elements has been collected, it
   calls the MAP function, passing it as arguments the constructed array,
   number of elements in it (can be less than NELEM on the last call),
   and call-specific DATA.  Iteration continues while MAP returns 0 and
   until all elements from the array have been visited.

   Mu_list_gmap returns 0 on success and a non-zero error code on failure.
   If MAP returns !0, its return value is propagated to the caller.
*/
   
int mu_list_gmap (mu_list_t list, mu_list_mapper_t map, size_t nelem,
		  void *data);
  
  
#define MU_LIST_MAP_OK     0x00
#define MU_LIST_MAP_SKIP   0x01
#define MU_LIST_MAP_STOP   0x02

/* List-to-list mapping.
   
   Apply the list mapping function MAP to each NELEM elements from the source
   LIST and use its return values to form a new list, which will be returned
   in RES.

   MAP gets pointers to the collected elements in its first argument (ITMV).
   Its second argument (ITMC) gives the number of elements filled in ARR.
   It can be less than NELEM on the last call to MAP.  The DATA pointer is
   passed as the 3rd argument.

   Return value from MAP governs the mapping process.  Unless it has the
   MU_LIST_MAP_SKIP bit set, the itmv[0] element is appended to the new
   list.  If it has MU_LIST_MAP_STOP bit set, iteration is stopped
   immediately and any remaining elements in LIST are ignored.
*/

int mu_list_map (mu_list_t list, mu_list_mapper_t map,
		 void *data, size_t nelem,
		 mu_list_t *res);
#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_LIST_H */
