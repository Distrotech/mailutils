/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

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

#include <mailutils/error.h>
#include <mailutils/sys/list.h>

/* Simple double (circular)link list.  We rollup our own because we
   need to be thread-safe.  */

int
mu_list_create (mu_list_t *plist)
{
  mu_list_t list;
  int status;
  if (plist == NULL)
    return EINVAL;
  list = calloc (sizeof (*list), 1);
  if (list == NULL)
    return ENOMEM;
  status = mu_refcount_create (&(list->refcount));
  if (list->refcount == NULL)
    {
      free (list);
      return status;
    }
  list->head.next = &(list->head);
  list->head.prev = &(list->head);
  list->count = 0;
  *plist = list;
  return 0;
}

int
mu_list_ref (mu_list_t list)
{
  if (list)
    return mu_refcount_inc (list->refcount);
  return 0;
}
void
mu_list_destroy (mu_list_t *plist)
{
  if (plist && *plist)
    {
      mu_list_t list = *plist;
      if (mu_refcount_dec (list->refcount) == 0)
	{
	  struct mu_list_data *current;
	  struct mu_list_data *previous;
	  mu_refcount_lock (list->refcount);
	  for (current = list->head.next; current != &(list->head);)
	    {
	      previous = current;
	      current = current->next;
	      free (previous);
	    }
	  mu_refcount_unlock (list->refcount);
	  mu_refcount_destroy (&list->refcount);
	  free (list);
	}
      *plist = NULL;
    }
}
int
mu_list_append (mu_list_t list, void *item)
{
  struct mu_list_data *ldata;
  struct mu_list_data *last;
  if (list == NULL)
    return EINVAL;
  last = list->head.prev;
  ldata = calloc (sizeof (*ldata), 1);
  if (ldata == NULL)
    return ENOMEM;
  ldata->item = item;
  mu_refcount_lock (list->refcount);
  ldata->next = &(list->head);
  ldata->prev = list->head.prev;
  last->next = ldata;
  list->head.prev = ldata;
  list->count++;
  mu_refcount_unlock (list->refcount);
  return 0;
}

int
mu_list_prepend (mu_list_t list, void *item)
{
  struct mu_list_data *ldata;
  struct mu_list_data *first;
  if (list == NULL)
    return EINVAL;
  first = list->head.next;
  ldata = calloc (sizeof (*ldata), 1);
  if (ldata == NULL)
    return ENOMEM;
  ldata->item = item;
  mu_refcount_lock (list->refcount);
  ldata->prev = &(list->head);
  ldata->next = list->head.next;
  first->prev = ldata;
  list->head.next = ldata;
  list->count++;
  mu_refcount_unlock (list->refcount);
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
  if (list == NULL || pcount == NULL)
    return EINVAL;
  mu_refcount_lock (list->refcount);
  *pcount = list->count;
  mu_refcount_unlock (list->refcount);
  return 0;
}

int
mu_list_remove (mu_list_t list, void *item)
{
  struct mu_list_data *current, *previous;
  int status = ENOENT;
  if (list == NULL)
    return EINVAL;
  mu_refcount_lock (list->refcount);
  for (previous = &(list->head), current = list->head.next;
       current != &(list->head); previous = current, current = current->next)
    {
      if ((int)current->item == (int)item)
	{
	  previous->next = current->next;
	  current->next->prev = previous;
	  free (current);
	  list->count--;
	  status = 0;
	  break;
	}
    }
  mu_refcount_unlock (list->refcount);
  return ENOENT;
}

int
mu_list_get (mu_list_t list, size_t indx, void **pitem)
{
  struct mu_list_data *current;
  size_t count;
  int status = ENOENT;
  if (list == NULL || pitem == NULL)
    return EINVAL;
  mu_refcount_lock (list->refcount);
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
  mu_refcount_unlock (list->refcount);
  return status;
}

static int  l_ref     __P ((iterator_t));
static void l_destroy __P ((iterator_t *));
static int  l_first   __P ((iterator_t));
static int  l_next    __P ((iterator_t));
static int  l_current __P ((iterator_t, void *));
static int  l_is_done __P ((iterator_t));

static struct _iterator_vtable l_i_vtable =
{
  /* Base.  */
  l_ref,
  l_destroy,

  l_first,
  l_next,
  l_current,
  l_is_done
};

int
mu_list_get_iterator (mu_list_t list, iterator_t *piterator)
{
  struct l_iterator *l_iterator;

  if (list == NULL || piterator == NULL)
    return MU_ERROR_INVALID_PARAMETER;

  l_iterator = malloc (sizeof *l_iterator);
  if (l_iterator == NULL)
    return MU_ERROR_NO_MEMORY;

  mu_refcount_create (&l_iterator->refcount);
  if (l_iterator->refcount == NULL)
    {
      free (l_iterator);
      return MU_ERROR_NO_MEMORY;
    }
  /* Incremente the reference count of the list.  */
  l_iterator->list = list;
  mu_list_ref (list);
  l_iterator->current = NULL;
  l_iterator->base.vtable = &l_i_vtable;
  *piterator = &l_iterator->base;
  return 0;
}

static int
l_ref (iterator_t iterator)
{
  struct l_iterator *l_iterator = (struct l_iterator *)iterator;
  return mu_refcount_inc (l_iterator->refcount);
}

static void
l_destroy (iterator_t *piterator)
{
  struct l_iterator *l_iterator = (struct l_iterator *)*piterator;
  if (mu_refcount_dec (l_iterator->refcount) == 0)
    {
      /* The reference was bumped when creating the iterator.  */
      mu_list_destroy (&l_iterator->list);
      mu_refcount_destroy (&l_iterator->refcount);
      free (l_iterator);
    }
}

static int
l_first (iterator_t iterator)
{
  struct l_iterator *l_iterator = (struct l_iterator *)iterator;
  mu_list_t list = l_iterator->list;
  if (list)
    {
      mu_refcount_lock (list->refcount);
      l_iterator->current = l_iterator->list->head.next;
      mu_refcount_unlock (list->refcount);
    }
  return 0;
}

static int
l_next (iterator_t iterator)
{
  struct l_iterator *l_iterator = (struct l_iterator *)iterator;
  mu_list_t list = l_iterator->list;
  if (list)
    {
      if (l_iterator->current)
	{
	  mu_refcount_lock (list->refcount);
	  l_iterator->current = l_iterator->current->next;
	  mu_refcount_unlock (list->refcount);
	}
      else
	l_first (iterator);
    }
  return 0;
}

static int
l_is_done (iterator_t iterator)
{
  struct l_iterator *l_iterator = (struct l_iterator *)iterator;
  int done = 0;
  mu_list_t list = l_iterator->list;
  if (list)
    {
      mu_refcount_lock (list->refcount);
      done = (l_iterator->current == &(list->head));
      mu_refcount_unlock (list->refcount);
    }
  return done;
}

static int
l_current (iterator_t iterator, void *item)
{
  struct l_iterator *l_iterator = (struct l_iterator *)iterator;
  mu_list_t list = l_iterator->list;
  if (list)
    {
      mu_refcount_lock (list->refcount);
      if (l_iterator->current)
	*((void **)item) = l_iterator->current->item;
      else
	*((void **)item) = NULL;
      mu_refcount_unlock (list->refcount);
    }
  return 0;
}
