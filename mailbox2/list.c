/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Library Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

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
list_create (list_t *plist)
{
  list_t list;
  int status;
  if (plist == NULL)
    return EINVAL;
  list = calloc (sizeof (*list), 1);
  if (list == NULL)
    return ENOMEM;
  status = monitor_create (&(list->lock));
  if (status != 0)
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
list_destroy (list_t list)
{
  if (list)
    {
      struct list_data *current;
      struct list_data *previous;
      monitor_lock (list->lock);
      for (current = list->head.next; current != &(list->head);)
	{
	  previous = current;
	  current = current->next;
	  free (previous);
	}
      monitor_unlock (list->lock);
      monitor_destroy (list->lock);
      free (list);
    }
  return 0;
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
  monitor_lock (list->lock);
  ldata->next = &(list->head);
  ldata->prev = list->head.prev;
  last->next = ldata;
  list->head.prev = ldata;
  list->count++;
  monitor_unlock (list->lock);
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
  monitor_lock (list->lock);
  ldata->prev = &(list->head);
  ldata->next = list->head.next;
  first->prev = ldata;
  list->head.next = ldata;
  list->count++;
  monitor_unlock (list->lock);
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
  if (list == NULL || pcount == NULL)
    return EINVAL;
  *pcount = list->count;
  return 0;
}

int
list_remove (list_t list, void *item)
{
  struct list_data *current, *previous;
  int status = ENOENT;
  if (list == NULL)
    return EINVAL;
  monitor_lock (list->lock);
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
  monitor_unlock (list->lock);
  return ENOENT;
}

int
list_get (list_t list, size_t index, void **pitem)
{
  struct list_data *current;
  size_t count;
  int status = ENOENT;
  if (list == NULL || pitem == NULL)
    return EINVAL;
  monitor_lock (list->lock);
  for (current = list->head.next, count = 0; current != &(list->head);
       current = current->next, count++)
    {
      if (count == index)
        {
          *pitem = current->item;
	  status = 0;
	  break;
        }
    }
  monitor_unlock (list->lock);
  return status;
}

static int  l_add_ref              __P ((iterator_t));
static int  l_release              __P ((iterator_t));
static int  l_destroy              __P ((iterator_t));
static int  l_first                __P ((iterator_t));
static int  l_next                 __P ((iterator_t));
static int  l_current              __P ((iterator_t, void *));
static int  l_is_done              __P ((iterator_t));

static struct _iterator_vtable l_i_vtable =
{
  /* Base.  */
  l_add_ref,
  l_release,
  l_destroy,

  l_first,
  l_next,
  l_current,
  l_is_done
};

int
list_get_iterator (list_t list, iterator_t *piterator)
{
  struct l_iterator *l_iterator;

  if (list == NULL || piterator == NULL)
    return MU_ERROR_INVALID_PARAMETER;

  l_iterator = malloc (sizeof *l_iterator);
  if (l_iterator == NULL)
    return MU_ERROR_NO_MEMORY;

  l_iterator->base.vtable = &l_i_vtable;
  l_iterator->ref = 1;
  l_iterator->list = list;
  l_iterator->current = NULL;
  monitor_create (&(l_iterator->lock));
  *piterator = &l_iterator->base;
  return 0;
}

static int
l_add_ref (iterator_t iterator)
{
  int status = 0;
  struct l_iterator *l_iterator = (struct l_iterator *)iterator;
  if (l_iterator)
    {
      monitor_lock (l_iterator->lock);
      status = ++l_iterator->ref;
      monitor_unlock (l_iterator->lock);
    }
  return status;
}

static int
l_release (iterator_t iterator)
{
  int status = 0;
  struct l_iterator *l_iterator = (struct l_iterator *)iterator;
  if (l_iterator)
    {
      monitor_lock (l_iterator->lock);
      status = --l_iterator->ref;
      if (status <= 0)
        {
          monitor_unlock (l_iterator->lock);
          l_destroy (iterator);
          return 0;
        }
      monitor_unlock (l_iterator->lock);
    }
  return status;
}

static int
l_destroy (iterator_t iterator)
{
  struct l_iterator *l_iterator = (struct l_iterator *)iterator;
  monitor_destroy (l_iterator->lock);
  free (l_iterator);
  return 0;
}

static int
l_first (iterator_t iterator)
{
  struct l_iterator *l_iterator = (struct l_iterator *)iterator;
  list_t list = l_iterator->list;
  if (list)
    {
      monitor_lock (list->lock);
      l_iterator->current = l_iterator->list->head.next;
      monitor_unlock (list->lock);
    }
  return 0;
}

static int
l_next (iterator_t iterator)
{
  struct l_iterator *l_iterator = (struct l_iterator *)iterator;
  list_t list = l_iterator->list;
  if (list)
    {
      if (l_iterator->current)
	{
	  monitor_lock (list->lock);
	  l_iterator->current = l_iterator->current->next;
	  monitor_unlock (list->lock);
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
  list_t list = l_iterator->list;
  if (list)
    {
      monitor_lock (list->lock);
      done = (l_iterator->current == &(list->head));
      monitor_unlock (list->lock);
    }
  return done;
}

static int
l_current (iterator_t iterator, void *item)
{
  struct l_iterator *l_iterator = (struct l_iterator *)iterator;
  list_t list = l_iterator->list;
  if (list)
    {
      monitor_lock (list->lock);
      if (l_iterator->current)
	*((void **)item) = l_iterator->current->item;
      else
	*((void **)item) = NULL;
      monitor_unlock (list->lock);
    }
  return 0;
}
