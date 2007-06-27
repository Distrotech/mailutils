/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2004, 2005, 2007 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library; if not, write to the
   Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301 USA */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <list0.h>
#include <iterator0.h>
#include <mailutils/errno.h>

int
mu_list_create (mu_list_t *plist)
{
  mu_list_t list;
  int status;

  if (plist == NULL)
    return MU_ERR_OUT_PTR_NULL;
  list = calloc (sizeof (*list), 1);
  if (list == NULL)
    return ENOMEM;
  status = mu_monitor_create (&(list->monitor), 0,  list);
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
mu_list_destroy (mu_list_t *plist)
{
  if (plist && *plist)
    {
      mu_list_t list = *plist;
      struct list_data *current;
      struct list_data *previous;

      mu_monitor_wrlock (list->monitor);
      for (current = list->head.next; current != &(list->head);)
	{
	  previous = current;
	  current = current->next;
	  if (list->destroy_item)
	    list->destroy_item (previous->item);
	  free (previous);
	}
      mu_monitor_unlock (list->monitor);
      mu_monitor_destroy (&(list->monitor), list);
      free (list);
      *plist = NULL;
    }
}

int
mu_list_append (mu_list_t list, void *item)
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
  mu_monitor_wrlock (list->monitor);
  ldata->next = &(list->head);
  ldata->prev = list->head.prev;
  last->next = ldata;
  list->head.prev = ldata;
  list->count++;
  mu_monitor_unlock (list->monitor);
  return 0;
}

int
mu_list_prepend (mu_list_t list, void *item)
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
  mu_monitor_wrlock (list->monitor);
  ldata->prev = &(list->head);
  ldata->next = list->head.next;
  first->prev = ldata;
  list->head.next = ldata;
  list->count++;
  mu_monitor_unlock (list->monitor);
  return 0;
}

int
mu_list_is_empty (mu_list_t list)
{
  size_t n = 0;

  mu_list_count (list, &n);
  return (n == 0);
}

int
mu_list_count (mu_list_t list, size_t *pcount)
{
  if (list == NULL)
    return EINVAL;
  if (pcount == NULL)
    return MU_ERR_OUT_PTR_NULL;
  *pcount = list->count;
  return 0;
}

mu_list_comparator_t
mu_list_set_comparator (mu_list_t list, mu_list_comparator_t comp)
{
  mu_list_comparator_t old_comp;

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
mu_list_locate (mu_list_t list, void *item, void **ret_item)
{
  struct list_data *current, *previous;
  mu_list_comparator_t comp;
  int status = MU_ERR_NOENT;

  if (list == NULL)
    return EINVAL;
  comp = list->comp ? list->comp : def_comp;
  mu_monitor_wrlock (list->monitor);
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
  mu_monitor_unlock (list->monitor);
  return status;
}

static int
_insert_item(mu_list_t list, struct list_data *current, void *new_item,
	     int insert_before)
{
  struct list_data *ldata = calloc (sizeof (*ldata), 1);
  if (ldata == NULL)
    return ENOMEM;

  ldata->item = new_item;
  if (insert_before)
    {
      ldata->prev = current->prev;
      ldata->next = current;
      if (current->prev != &list->head)
	current->prev->next = ldata;
      else
	list->head.next = ldata;

      current->prev = ldata;
    }
  else
    {
      ldata->next = current->next;
      ldata->prev = current;
      
      if (current->next != &list->head)
	current->next->prev = ldata;
      else
	list->head.prev = ldata;

      current->next = ldata;
    }
  
  list->count++;
  return 0;
}

int
mu_list_insert (mu_list_t list, void *item, void *new_item, int insert_before)
{
  struct list_data *current;
  mu_list_comparator_t comp;
  int status = MU_ERR_NOENT;

  if (list == NULL)
    return EINVAL;
  comp = list->comp ? list->comp : def_comp;

  mu_monitor_wrlock (list->monitor);
  for (current = list->head.next;
       current != &(list->head);
       current = current->next)
    {
      if (comp (current->item, item) == 0)
	{
	  status = _insert_item (list, current, new_item, insert_before);
	  break;
	}
    }
  mu_monitor_unlock (list->monitor);
  return status;
}

int
mu_list_remove (mu_list_t list, void *item)
{
  struct list_data *current, *previous;
  mu_list_comparator_t comp;
  int status = MU_ERR_NOENT;

  if (list == NULL)
    return EINVAL;
  comp = list->comp ? list->comp : def_comp;
  mu_monitor_wrlock (list->monitor);
  for (previous = &(list->head), current = list->head.next;
       current != &(list->head); previous = current, current = current->next)
    {
      if (comp (current->item, item) == 0)
	{
	  mu_iterator_advance (list->itr, current);
	  previous->next = current->next;
	  current->next->prev = previous;
	  free (current);
	  list->count--;
	  status = 0;
	  break;
	}
    }
  mu_monitor_unlock (list->monitor);
  return status;
}

int
mu_list_replace (mu_list_t list, void *old_item, void *new_item)
{
  struct list_data *current, *previous;
  mu_list_comparator_t comp;
  int status = MU_ERR_NOENT;

  if (list == NULL)
    return EINVAL;
  comp = list->comp ? list->comp : def_comp;
  mu_monitor_wrlock (list->monitor);
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
  mu_monitor_unlock (list->monitor);
  return status;
}

int
mu_list_get (mu_list_t list, size_t indx, void **pitem)
{
  struct list_data *current;
  size_t count;
  int status = MU_ERR_NOENT;

  if (list == NULL)
    return EINVAL;
  if (pitem == NULL)
    return MU_ERR_OUT_PTR_NULL;
  mu_monitor_rdlock (list->monitor);
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
  mu_monitor_unlock (list->monitor);
  return status;
}

int
mu_list_do (mu_list_t list, mu_list_action_t * action, void *cbdata)
{
  mu_iterator_t itr;
  int status = 0;

  if (list == NULL || action == NULL)
    return EINVAL;
  status = mu_list_get_iterator(list, &itr);
  if (status)
    return status;
  for (mu_iterator_first (itr); !mu_iterator_is_done (itr); mu_iterator_next (itr))
    {
      void *item;
      mu_iterator_current (itr, &item);
      if ((status = action (item, cbdata)))
	break;
    }
  mu_iterator_destroy (&itr);
  return status;
}

int
mu_list_set_destroy_item (mu_list_t list, void (*destroy_item)(void *item))
{
  if (list == NULL)
    return EINVAL;
  list->destroy_item = destroy_item;
  return 0;
}

int
mu_list_to_array (mu_list_t list, void **array, size_t count, size_t *pcount)
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
	array[i++] = current->item;
    }
  if (pcount)
    *pcount = total;
  return 0;
}


/* Iterator interface */

struct list_iterator
{
  mu_list_t list;
  struct list_data *cur;
};

static int
first (void *owner)
{
  struct list_iterator *itr = owner;
  itr->cur = itr->list->head.next;
  return 0;
}

static int
next (void *owner)
{
  struct list_iterator *itr = owner;
  itr->cur = itr->cur->next;
  return 0;
}

static int
getitem (void *owner, void **pret, const void **pkey)
{
  struct list_iterator *itr = owner;
  *pret = itr->cur->item;
  if (pkey)
    *pkey = NULL;
  return 0;
}

static int
finished_p (void *owner)
{
  struct list_iterator *itr = owner;
  return itr->cur == &itr->list->head;
}

static int
destroy (mu_iterator_t iterator, void *data)
{
  struct list_iterator *itr = data;
  mu_iterator_detach (&itr->list->itr, iterator);
  free (data);
  return 0;
}

static int
curitem_p (void *owner, void *item)
{
  struct list_iterator *itr = owner;
  return itr->cur == item;
}

static int
list_data_dup (void **ptr, void *owner)
{
  *ptr = malloc (sizeof (struct list_iterator));
  if (*ptr == NULL)
    return ENOMEM;
  memcpy (*ptr, owner, sizeof (struct list_iterator));
  return 0;
}

int
mu_list_get_iterator (mu_list_t list, mu_iterator_t *piterator)
{
  mu_iterator_t iterator;
  int status;
  struct list_iterator *itr;

  if (!list)
    return EINVAL;

  itr = calloc (1, sizeof *itr);
  if (!itr)
    return ENOMEM;
  itr->list = list;
  itr->cur = NULL;

  status = mu_iterator_create (&iterator, itr);
  if (status)
    {
      free (itr);
      return status;
    }

  mu_iterator_set_first (iterator, first);
  mu_iterator_set_next (iterator, next);
  mu_iterator_set_getitem (iterator, getitem);
  mu_iterator_set_finished_p (iterator, finished_p);
  mu_iterator_set_curitem_p (iterator, curitem_p);
  mu_iterator_set_destroy (iterator, destroy);
  mu_iterator_set_dup (iterator, list_data_dup);

  mu_iterator_attach (&list->itr, iterator);

  *piterator = iterator;
  return 0;
}
