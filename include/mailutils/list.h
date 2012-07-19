/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2000, 2005, 2007-2008, 2010-2012 Free Software
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

  /* ************************************************* */
  /* List creation and destruction                     */
  /* ************************************************* */
int mu_list_create (mu_list_t *_plist);
void mu_list_destroy (mu_list_t *_plist);
  /* Remove all elements from the list, reclaiming the associated memory
     if necessary (see mu_list_set_destroy_item), but don't destroy the
     list itself. */
void mu_list_clear (mu_list_t _list);

  /* ************************************************* */
  /* List Comparators                                  */
  /* ************************************************* */
  
  /* A comparator is associated with each list, which is used to compare
     two elements for equality.  The comparator is a function of
     mu_list_comparator_t type, which returns 0 if its arguments are equal
     and non-zero otherwise: */
typedef int (*mu_list_comparator_t) (const void*, const void*);
  /* The default comparator function compares two pointers for equality. */
int _mu_list_ptr_comparator (const void*, const void*);
  /* Set _cmp as a new comparator function for _list.  Return old
     comparator. */
mu_list_comparator_t mu_list_set_comparator (mu_list_t _list,
					     mu_list_comparator_t _cmp);
  /* Returns in _pcmp a pointer to the comparator function of _list: */
int mu_list_get_comparator (mu_list_t _list, mu_list_comparator_t *_pcmp);

  /* ************************************************* */
  /* Memory management                                 */
  /* ************************************************* */

  /* The destroy function is responsible for deallocating a list element.
     By default, it is not set. */
typedef void (*mu_list_destroy_item_t) (void *);

  /* An often used destroy function.  It simply calls free(3) over the
     _item. */
void mu_list_free_item (void *_item);

  /* Sets the destroy function for _list and returns old destroy
     function: */
mu_list_destroy_item_t mu_list_set_destroy_item (mu_list_t _list,
			   mu_list_destroy_item_t _destroy_item);

  /* ************************************************* */
  /* List informational functions:                     */
  /* ************************************************* */

  /* Return true if _list has no items. */
int mu_list_is_empty (mu_list_t _list);
  /* Write the number of items currently in _list to the memory location
     pointed to by _pcount. */
int mu_list_count    (mu_list_t _list, size_t *_pcount);

  /* ************************************************* */
  /* Functions for retrieving list elements:           */
  /* ************************************************* */
  
  /* Get _indexth element from the list and store it in _pitem. */
int mu_list_get      (mu_list_t _list, size_t _index, void **_pitem);
  /* Retrieve first element of _list */
int mu_list_head (mu_list_t _list, void **_pitem);
  /* Retrieve last element of _list */
int mu_list_tail (mu_list_t _list, void **_pitem);
  
  /* Store at most _count first elements from _list in the _array.  Unless
     _pcount is NULL, fill it with the actual number of elements copied. */
int mu_list_to_array (mu_list_t _list, void **_array, size_t _count,
		      size_t *_pcount);
  /* Look for the element matching _item (in the sense of the item comparator
     method, see mu_list_set_comparator) and return it in the memory location
     pointed to by _ret_item. */
int mu_list_locate   (mu_list_t list, void *item, void **ret_item);
  
  /* ************************************************* */
  /* Functions for adding and removing elements:       */
  /* ************************************************* */
  
  /* Add _item to the tail of the _list. */
int mu_list_append   (mu_list_t _list, void *_item);
  /* Add _item to the head of the _list */
int mu_list_prepend  (mu_list_t _list, void *_item);
  /* Insert _item after _new_item in _list (or before it, if _insert_before
     is 1. */
int mu_list_insert   (mu_list_t _list, void *_item, void *_new_item, 
                      int _insert_before);
  /* Remove _item from _list and deallocate any memory associated with it
     using the `destroy_item' method. */
int mu_list_remove   (mu_list_t _list, void *_item);
  /* A non-destructive version of mu_list_remove: removes the _item but does
     not deallocate it. */
int mu_list_remove_nd  (mu_list_t _list, void *_item);
  /* Remove Nth element from the list. */
int mu_list_remove_nth (mu_list_t list, size_t n);
  /* Remove Nth element from the list, non-destructive. */
int mu_list_remove_nth_nd (mu_list_t list, size_t n);
  /* Find _old_item in _list and if found, replace it with _new_item,
     deallocating the removed item via the `destroy_item' method. */
int mu_list_replace  (mu_list_t _list, void *_old_item, void *_new_item);
  /* A non-destructive version of mu_list_replace: the removed item is not
     deallocated. */
int mu_list_replace_nd (mu_list_t _list, void *_old_item, void *_new_item);

  /* ************************************************* */
  /* LIFO Access                                       */
  /* ************************************************* */
int mu_list_push (mu_list_t list, void *item);
int mu_list_pop (mu_list_t list, void **item);
  

  /* ************************************************* */
  /* Interation over lists.                            */
  /* ************************************************* */

  /* Get iterator for this _list and return it in _pitr, if successful. */
int mu_list_get_iterator (mu_list_t _list, mu_iterator_t *_pitr);

  /* A general-purpose iteration function.  When called, _item points to
     the item currently visited and _data points to call-specific data. */
typedef int (*mu_list_action_t) (void *_item, void *_data);

  /* Execute _action for each element in _list.  Use _data as the call-specific
     data.  If _dir is 0, traverse the list from head to tail.  If it is 1,
     traverse it in the reverse direction */
int mu_list_foreach_dir (mu_list_t _list, int _dir, mu_list_action_t _action,
			 void *_cbdata);
  /* Same as mu_list_foreach_dir with _dir==0. */
int mu_list_foreach (mu_list_t _list, mu_list_action_t _action, void *_data);
  /* A historical alias to the above. */
int mu_list_do (mu_list_t, mu_list_action_t, void *) MU_DEPRECATED;
  

  /* ************************************************* */
  /* Functions for combining two lists.                */
  /* ************************************************* */

  /* Move elements from _new_list to _list, adding them to the tail of
     the latter. */
void mu_list_append_list (mu_list_t _list, mu_list_t _new_list);
  /* Move elements from _new_list to _list, adding them to the head of
     the latter. */
void mu_list_prepend_list (mu_list_t _list, mu_list_t _new_list);

  /* Move data from _new_list to _list, inserting them after the
     element matching _anchor (in the sense of _list's comparator
     function).  If _insert_before is 1, items are inserted before
     _achor instead. */
int mu_list_insert_list (mu_list_t _list, void *_anchor,
			 mu_list_t _new_list,
			 int _insert_before);

  /* Compute an intersection of two lists (_list1 and _list2) and return
     it in _pdest.  The resulting list contains elements from _list1 that
     are also encountered in _list2 (as per comparison function of
     the latter).
     
     If _dup_item is not NULL, it is called to create copies of
     items to be stored in _pdest.  In this case, the destroy_item
     function of _list2 is also attached to _pdest.

     The _dup_item parameters are: a pointer for returned data, the
     original item and call-specific data.  The _dup_data 
     argument is passed as the 3rd argument in each call to _dup_item.
     
     If _dup_item is NULL, pointers to elements are stored and
     no destroy_item function is assigned. */
int mu_list_intersect_dup (mu_list_t *_pdest,
			   mu_list_t _list1, mu_list_t _list2,
			   int (*_dup_item) (void **, void *, void *),
			   void *_dup_data);
  
  /* Same as mu_list_intersect_dup with _dup_item = NULL */
int mu_list_intersect (mu_list_t *, mu_list_t, mu_list_t);

  /* ************************************************* */
  /* List slicing                                      */
  /* ************************************************* */
  /* Create a new list from elements of _list located between
     indices in _posv.  Return the result in _pdest.  
     The resulting list will contain elements between _posv[0] and
     _posv[1], _posv[2] and _posv[3], ..., _posv[_posc-2]
     and _posv[_posc-1], inclusive.  If _posc is an odd number, an extra
     element with the value [count-1] (where count is the number of
     elements in _list) is implied.

     The elements in _posv are sorted in ascending order prior to use. 
     
     See mu_list_intersect_dup for a description of _dup_item and
     _dup_data */
int mu_list_slice_dup (mu_list_t *_pdest, mu_list_t _list,
		       size_t *_posv, size_t _posc,
		       int (*_dup_item) (void **, void *, void *),
		       void *_dup_data);
  /* Same as mu_list_slice_dup invoked with _dup_item=NULL */
int mu_list_slice (mu_list_t *_pdest, mu_list_t _list,
		   size_t *_posv, size_t _posc);
  
  /* Two functions for the most common case: */
  /* Create a slice containing elements between indices _from and
     _to in the _list.

     See mu_list_intersect_dup for a description of _dup_item and
     _dup_data */
int mu_list_slice2_dup (mu_list_t *_pdest, mu_list_t _list,
			size_t _from, size_t _to,
			int (*_dup_item) (void **, void *, void *),
			void *_dup_data);
  /* Same as mu_list_slice2_dup with _dup_item=NULL */
int mu_list_slice2 (mu_list_t *_pdest, mu_list_t _list,
                    size_t _from, size_t _to);
  

  /* ************************************************* */
  /* List mapper functions                             */
  /* ************************************************* */
  
typedef int (*mu_list_mapper_t) (void **_itmv, size_t _itmc, void *_call_data);

/* A generalized list mapping function.

   Mu_list_gmap iterates over the _list, gathering its elements in an
   array of type void **.  Each time _nelem elements has been collected,
   it calls the _map function, passing it as arguments the constructed array,
   the number of elements in it (which can be less than _nelem on the last
   call), and call-specific _data.  Iteration continues while _map returns 0
   and until all elements from the array have been visited.

   Mu_list_gmap returns 0 on success and a non-zero error code on failure.
   If _map returns non-zero, its return value is propagated to the caller.

   The _map function is not allowed to alter the _list.
*/
   
int mu_list_gmap (mu_list_t _list, mu_list_mapper_t _map, size_t _nelem,
		  void *_data);
  
  
#define MU_LIST_MAP_OK     0x00
#define MU_LIST_MAP_SKIP   0x01
#define MU_LIST_MAP_STOP   0x02

/* List-to-list mapping.
   
   Apply the list mapping function _map to each _nelem elements from the source
   _list and use its return values to form a new list, which will be returned
   in _res.

   The _map function gets pointers to the collected elements in its first
   argument (_itmv).  Its second argument (_itmc) keeps the number of elements
   filled in there.  It can be less than _nelem on the last call to _map.
   The _data pointer is passed as the 3rd argument.

   The return value from _map governs the mapping process.  Unless it has the
   MU_LIST_MAP_SKIP bit set, the _itmv[0] element is appended to the new
   list.  If it has MU_LIST_MAP_STOP bit set, iteration is stopped
   immediately and any remaining elements in _list are ignored.

   The mapper function (_map) is not allowed to alter the _list.
*/

int mu_list_map (mu_list_t _list, mu_list_mapper_t _map,
		 void *_data, size_t _nelem,
		 mu_list_t *_res);

  /* ************************************************* */
  /* List folding                                      */
  /* ************************************************* */

typedef int (*mu_list_folder_t) (void *_item, void *_data,
				 void *_prev, void **_ret);

  /* mu_list_fold iterates over list elements from first to last.
     For each element it calls _fold with the following arguments:

       _item     -  the current list element,
       _data     -  call-specific data,
       _prev     -  on the first call, _init; on subsequent calls,
                    points to the value returned from the previous call
		    to _fold in the _ret varialble,
       _ret      -  memory location where to store the result of this
                    call.

     When all elements have been visited, mu_list_fold stores the result
     of the last _fold invocation (as returned in *_ret) in the memory
     location pointed to by _return_value.

     If _fold returns a non-zero value, mu_list_fold stops iteration and
     returns this value.  The *_return_value is filled in this case as
     well.

     Possible return codes:

       0                   - success,
       EINVAL              - _list or _fold is NULL,
       MU_ERR_OUT_PTR_NULL - _return_code is NULL
       other value         - non-zero value returned by _fold.

     The _fold function is not allowed to alter the list it is being applied
     to.
       
     The mu_list_rfold acts similarly, except that it iterates over list
     elements from last to first.
  */
int mu_list_fold (mu_list_t _list, mu_list_folder_t _fold, void *_data,
		  void *_init, void *_return_value);
int mu_list_rfold (mu_list_t _list, mu_list_folder_t _fold, void *_data,
		   void *_init, void *_return_value);

  /* ************************************************* */
  /* Sorting                                           */
  /* ************************************************* */

  /* Sort _list using quicksort algorithm.  Use _comp to compare elements.
     If it is NULL, use the comparator method of _list.

     Comparator must return 0 if the two elements are equal, -1 if
     first of them is less than the second, and +1 otherwise.
  */
void mu_list_sort (mu_list_t _list, mu_list_comparator_t _comp);
  
#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_LIST_H */
