/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2004 Free Software Foundation, Inc.

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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <errno.h>
#include <stdlib.h>

#include <list0.h>
#include <iterator0.h>
#include <mailutils/errno.h>

int
list_create (list_t *plist)
{
  list_t list;
  int status;
  
  if (plist == NULL)
    return MU_ERR_OUT_PTR_NULL;
  list = calloc (sizeof (*list), 1);
  if (list == NULL)
    return ENOMEM;
  status = monitor_create (&(list->monitor), 0,  list);
  if (status != 0)
    {
      free (list);
      return status;
    }
  list->head.next = &(list->head);
  list->head.prev = &(list->head);
  *plist = list;
  return 0;
}

void
list_destroy (list_t *plist)
{
  if (plist && *plist)
    {
      list_t list = *plist;
      struct list_data *current;
      struct list_data *previous;
      
      monitor_wrlock (list->monitor);
      for (current = list->head.next; current != &(list->head);)
	{
	  previous = current;
	  current = current->next;
	  if (list->destroy_item)
	    list->destroy_item (previous->item);
	  free (previous);
	}
      monitor_unlock (list->monitor);
      monitor_destroy (&(list->monitor), list);
      free (list);
      *plist = NULL;
    }
}

int
list_append (list_t list, void *item)
{
  struct list_data *ldata;
  struct list_data *last;
  
  if (list == NULL)
    return EINVAL;
  last = list->head.prev;
  ldata = calloc (sizeof (*ldata), 1);
  if (ldata == NULL)
    return ENOMEM;
  ldata->item = item;
  monitor_wrlock (list->monitor);
  ldata->next = &(list->head);
  ldata->prev = list->head.prev;
  last->next = ldata;
  list->head.prev = ldata;
  list->count++;
  monitor_unlock (list->monitor);
  return 0;
}

int
list_prepend (list_t list, void *item)
{
  struct list_data *ldata;
  struct list_data *first;
  
  if (list == NULL)
    return EINVAL;
  first = list->head.next;
  ldata = calloc (sizeof (*ldata), 1);
  if (ldata == NULL)
    return ENOMEM;
  ldata->item = item;
  monitor_wrlock (list->monitor);
  ldata->prev = &(list->head);
  ldata->next = list->head.next;
  first->prev = ldata;
  list->head.next = ldata;
  list->count++;
  monitor_unlock (list->monitor);
  return 0;
}

int
list_is_empty (list_t list)
{
  size_t n = 0;
  
  list_count (list, &n);
  return (n == 0);
}

int
list_count (list_t list, size_t *pcount)
{
  if (list == NULL)
    return EINVAL;
  if (pcount == NULL)
    return MU_ERR_OUT_PTR_NULL;
  *pcount = list->count;
  return 0;
}

list_comparator_t
list_set_comparator (list_t list, list_comparator_t comp)
{
  list_comparator_t old_comp;
  
  if (list == NULL)
    return NULL;
  old_comp = list->comp;
  list->comp = comp;
  return old_comp;
}  

static int
def_comp (const void *item, const void *value)
{
  return item != value;
}

int
list_locate (list_t list, void *item, void **ret_item)
{
  struct list_data *current, *previous;
  list_comparator_t comp;
  int status = MU_ERR_NOENT;
  
  if (list == NULL)
    return EINVAL;
  comp = list->comp ? list->comp : def_comp;
  monitor_wrlock (list->monitor);
  for (previous = &(list->head), current = list->head.next;
       current != &(list->head); previous = current, current = current->next)
    {
      if (comp (current->item, item) == 0)
	{
	  if (ret_item)
	    *ret_item = current->item;
	  status = 0;
	  break;
	}
    }
  monitor_unlock (list->monitor);
  return status;
}

int
list_insert (list_t list, void *item, void *new_item)
{
  struct list_data *current;
  list_comparator_t comp;
  int status = MU_ERR_NOENT;
  
  if (list == NULL)
    return EINVAL;
  comp = list->comp ? list->comp : def_comp;

  monitor_wrlock (list->monitor);
  for (current = list->head.next;
       current != &(list->head);
       current = current->next)
    {
      if (comp (current->item, item) == 0)
	{
	  struct list_data *ldata = calloc (sizeof (*ldata), 1);
	  if (ldata == NULL)
	    return ENOMEM;
	  
	  ldata->item = new_item;
	  ldata->next = current->next;
	  ldata->prev = current;
	  if (current->next)
	    current->next->prev = ldata;
	  else
	    list->head.prev = ldata;

	  if (current == list->head.next)
	    current = list->head.next = ldata;
	  else
	    current->next = ldata;
	  
	  list->count++;
	  status = 0;
	  break;
	}
    }
  monitor_unlock (list->monitor);
  return status;
}

int
list_remove (list_t list, void *item)
{
  struct list_data *current, *previous;
  list_comparator_t comp;
  int status = MU_ERR_NOENT;
  
  if (list == NULL)
    return EINVAL;
  comp = list->comp ? list->comp : def_comp;
  monitor_wrlock (list->monitor);
  for (previous = &(list->head), current = list->head.next;
       current != &(list->head); previous = current, current = current->next)
    {
      if (comp (current->item, item) == 0)
	{
	  iterator_advance (list->itr, current);
	  previous->next = current->next;
	  current->next->prev = previous;
	  free (current);
	  list->count--;
	  status = 0;
	  break;
	}
    }
  monitor_unlock (list->monitor);
  return status;
}

int
list_replace (list_t list, void *old_item, void *new_item)
{
  struct list_data *current, *previous;
  list_comparator_t comp;
  int status = MU_ERR_NOENT;
  
  if (list == NULL)
    return EINVAL;
  comp = list->comp ? list->comp : def_comp;
  monitor_wrlock (list->monitor);
  for (previous = &(list->head), current = list->head.next;
       current != &(list->head); previous = current, current = current->next)
    {
      if (comp (current->item, old_item) == 0)
	{
	  current->item = new_item;
	  status = 0;
	  break;
	}
    }
  monitor_unlock (list->monitor);
  return status;
}

int
list_get (list_t list, size_t indx, void **pitem)
{
  struct list_data *current;
  size_t count;
  int status = MU_ERR_NOENT;
  
  if (list == NULL)
    return EINVAL;
  if (pitem == NULL)
    return MU_ERR_OUT_PTR_NULL;
  monitor_rdlock (list->monitor);
  for (current = list->head.next, count = 0; current != &(list->head);
       current = current->next, count++)
    {
      if (count == indx)
        {
          *pitem = current->item;
	  status = 0;
	  break;
        }
    }
  monitor_unlock (list->monitor);
  return status;
}

int
list_do (list_t list, list_action_t * action, void *cbdata)
{
  iterator_t itr;
  int status = 0;
  
  if (list == NULL || action == NULL)
    return EINVAL;
  status = iterator_create(&itr, list);
  if (status)
    return status;
  for (iterator_first (itr); !iterator_is_done (itr); iterator_next (itr))
    {
      void *item;
      iterator_current (itr, &item);
      if ((status = action (item, cbdata)))
	break;
    }
  iterator_destroy (&itr);
  return status;
}

int
list_set_destroy_item (list_t list, void (*destroy_item)(void *item))
{
  if (list == NULL)
    return EINVAL;
  list->destroy_item = destroy_item;
}

int
list_to_array (list_t list, void **array, size_t count, size_t *pcount)
{
  size_t total = 0;

  if (!list)
    return EINVAL;
  
  total = (count < list->count) ? count : list->count;

  if (array)
    {
      size_t i;
      struct list_data *current;
      
      for (i = 0, current = list->head.next;
	   i < total && current != &(list->head); current = current->next)
	array[i] = current->item;
    }
  if (pcount)
    *pcount = total; 
  return 0;
}
