/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2011-2012 Free Software Foundation, Inc.

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
#include <mailutils/errno.h>
#include <mailutils/sys/list.h>

static void
_list_append_entry (struct _mu_list *list, struct list_data *ent)
{
  ent->prev = list->head.prev ? list->head.prev : &list->head;
  ent->next = &list->head;
  if (list->head.prev)
    list->head.prev->next = ent;
  else
    list->head.next = ent;
  list->head.prev = ent;
  list->count++;
}

static void
_list_qsort (mu_list_t list, mu_list_comparator_t cmp)
{
  struct list_data *cur, *middle;
  struct _mu_list high_list, low_list;
  int rc;

  if (list->count < 2)
    return;
  if (list->count == 2)
    {
      if (cmp (list->head.prev->item, list->head.next->item) < 0)
	{
	  cur = list->head.prev;
	  list->head.prev = list->head.next;
	  list->head.next = cur;
	  
	  list->head.next->prev = &list->head;
	  list->head.next->next = list->head.prev;
	  
	  list->head.prev->next = &list->head;
	  list->head.prev->prev = list->head.next;
	}
      return;
    }
  
  cur = list->head.next;
  do {
    cur = cur->next;
    if (cur == &list->head)
      return;
  } while ((rc = cmp (list->head.next->item, cur->item)) == 0);

  /* Select the lower of the two as the middle value */
  middle = (rc > 0) ? cur : list->head.next;

  /* Split into two sublists */
  memset (&high_list, 0, sizeof (high_list));
  memset (&low_list, 0, sizeof (low_list));

  for (cur = list->head.next; cur != &list->head; )
    {
      struct list_data *next = cur->next;
      cur->next = NULL;

      if (cmp (middle->item, cur->item) < 0)
	_list_append_entry (&high_list, cur);
      else
	_list_append_entry (&low_list, cur);
      cur = next;
    }

  /* Sort both sublists recursively */
  _list_qsort (&low_list, cmp);
  _list_qsort (&high_list, cmp);

  /* Join both lists in order */
  if (low_list.head.prev)
    cur = low_list.head.prev;
  else
    cur = &low_list.head;
  cur->next = high_list.head.next;
  if (high_list.head.next)
    high_list.head.next->prev = cur;
  
  low_list.head.prev = high_list.head.prev;
  high_list.head.prev = &low_list.head;
  low_list.count += high_list.count;
    
  /* Return the resulting list */
  list->head = low_list.head;
  if (list->head.next)
    list->head.next->prev = &list->head;
  if (list->head.prev)
    list->head.prev->next = &list->head;
}

void
mu_list_sort (mu_list_t list, mu_list_comparator_t comp)
{
  if (list)
    _list_qsort (list, comp ? comp : list->comp);
}
