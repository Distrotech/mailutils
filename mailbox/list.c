/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000 Free Software Foundation, Inc.

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

#include <misc.h>
#include <list0.h>

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
  status = RWLOCK_INIT (&(list->rwlock), NULL);
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
      RWLOCK_WRLOCK (&(list->rwlock));
      for (current = list->head.next; current != &(list->head);)
	{
	  previous = current;
	  current = current->next;
	  free (previous);
	}
      RWLOCK_UNLOCK (&(list->rwlock));
      RWLOCK_DESTROY (&(list->rwlock));
      free (list);
      *plist = NULL;
    }
}

int
list_append (list_t list, void *item)
{
  struct list_data *ldata;
  struct list_data *last = list->head.prev;
  ldata = calloc (sizeof (*ldata), 1);
  if (ldata == NULL)
    return ENOMEM;
  ldata->item = item;
  RWLOCK_WRLOCK (&(list->rwlock));
  ldata->next = &(list->head);
  ldata->prev = list->head.prev;
  last->next = ldata;
  list->head.prev = ldata;
  list->count++;
  RWLOCK_UNLOCK (&(list->rwlock));
  return 0;
}

int
list_prepend (list_t list, void *item)
{
  struct list_data *ldata;
  struct list_data *first = list->head.next;
  ldata = calloc (sizeof (*ldata), 1);
  if (ldata == NULL)
    return ENOMEM;
  ldata->item = item;
  RWLOCK_WRLOCK (&(list->rwlock));
  ldata->prev = &(list->head);
  ldata->next = list->head.next;
  first->prev = ldata;
  list->head.next = ldata;
  list->count++;
  RWLOCK_UNLOCK (&(list->rwlock));
  return 0;
}

int
list_is_empty (list_t list)
{
  size_t n = 0;
  list_count (list, &n);
  return n;
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
  if (list == NULL)
    return EINVAL;
  RWLOCK_WRLOCK (&(list->rwlock));
  for (previous = &(list->head), current = list->head.next;
       current != &(list->head); previous = current, current = current->next)
    {
      if ((int)current->item == (int)item)
	{
	  previous->next = current->next;
	  current->next->prev = previous;
	  free (current);
	  list->count--;
	  RWLOCK_UNLOCK (&(list->rwlock));
	  return 0;
	}
    }
  RWLOCK_UNLOCK (&(list->rwlock));
  return ENOENT;
}

int
list_get (list_t list, size_t index, void **pitem)
{
  struct list_data *current;
  size_t count;
  if (list == NULL || pitem == NULL)
    return EINVAL;
  RWLOCK_RDLOCK (&(list->rwlock));
  for (current = list->head.next, count = 0; current != &(list->head);
       current = current->next, count++)
    {
      if (count == index)
        {
          *pitem = current->item;
	  RWLOCK_UNLOCK (&(list->rwlock));
          return 0;
        }
    }
  RWLOCK_UNLOCK (&(list->rwlock));
  return ENOENT;
}
