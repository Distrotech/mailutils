/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2001, 2004-2005, 2007-2008, 2010-2012 Free
   Software Foundation, Inc.

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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <mailutils/sys/list.h>
#include <mailutils/sys/iterator.h>
#include <mailutils/errno.h>

/* Iterator interface */

struct list_iterator
{
  mu_list_t list;
  struct list_data *cur;
  int backwards; /* true if iterating backwards */
};

static int
first (void *owner)
{
  struct list_iterator *itr = owner;
  if (itr->backwards)
    itr->cur = itr->list->head.prev;
  else
    itr->cur = itr->list->head.next;
  return 0;
}

static int
next (void *owner)
{
  struct list_iterator *itr = owner;
  if (itr->backwards)
    itr->cur = itr->cur->prev;
  else
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
delitem (void *owner, void *item)
{
  struct list_iterator *itr = owner;
  return itr->cur == item ? MU_ITR_DELITEM_NEXT : MU_ITR_DELITEM_NOTHING;
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

static int
list_itrctl (void *owner, enum mu_itrctl_req req, void *arg)
{
  struct list_iterator *itr = owner;
  mu_list_t list = itr->list;
  struct list_data *ptr;
  
  switch (req)
    {
    case mu_itrctl_tell:
      /* Return current position in the object */
      if (itr->cur == NULL)
	return MU_ERR_NOENT;
      else
	{
	  size_t count;

	  for (count = 0, ptr = list->head.next; ptr != &list->head;
	       ptr = ptr->next, count++)
	    {
	      if (ptr == itr->cur)
		{
		  *(size_t*)arg = count;
		  return 0;
		}
	    }
	  return MU_ERR_NOENT;
	}
      break;
      
    case mu_itrctl_delete:
    case mu_itrctl_delete_nd:
      /* Delete current element */
      if (itr->cur == NULL)
	return MU_ERR_NOENT;
      else
	{
	  struct list_data *prev;
	
	  ptr = itr->cur;
	  prev = ptr->prev;
	
	  mu_iterator_delitem (list->itr, ptr);
	  prev->next = ptr->next;
	  ptr->next->prev = prev;
	  if (req == mu_itrctl_delete)
	    DESTROY_ITEM (list, ptr);
	  free (ptr);
	  list->count--;
	}
      break;
      
    case mu_itrctl_replace:
    case mu_itrctl_replace_nd:
      /* Replace current element */
      if (itr->cur == NULL)
	return MU_ERR_NOENT;
      if (!arg)
	return EINVAL;
      ptr = itr->cur;
      if (req == mu_itrctl_replace)
	  DESTROY_ITEM (list, ptr);
      ptr = itr->cur;
      ptr->item = arg;
      break;
      
    case mu_itrctl_insert:
      /* Insert new element in the current position */
      if (itr->cur == NULL)
	return MU_ERR_NOENT;
      if (!arg)
	return EINVAL;
      return _mu_list_insert_item (list, itr->cur, arg, 0);

    case mu_itrctl_insert_list:
      /* Insert a list of elements */
      if (itr->cur == NULL)
	return MU_ERR_NOENT;
      if (!arg)
	return EINVAL;
      else
	{
	  mu_list_t new_list = arg;
	  _mu_list_insert_sublist (list, itr->cur,
				   new_list->head.next, new_list->head.prev,
				   new_list->count,
				   0);
	  _mu_list_clear (new_list);
	}
      break;

    case mu_itrctl_qry_direction:
      if (!arg)
	return EINVAL;
      else
	*(int*)arg = itr->backwards;
      break;

    case mu_itrctl_set_direction:
      if (!arg)
	return EINVAL;
      else
	itr->backwards = !!*(int*)arg;
      break;
      
    default:
      return ENOSYS;
    }
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
  mu_iterator_set_delitem (iterator, delitem);
  mu_iterator_set_destroy (iterator, destroy);
  mu_iterator_set_dup (iterator, list_data_dup);
  mu_iterator_set_itrctl (iterator, list_itrctl);
  
  mu_iterator_attach (&list->itr, iterator);

  *piterator = iterator;
  return 0;
}
