/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000 Free Software Foundation, Inc.

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

int
iterator_create (iterator_t *piterator, list_t list)
{
  iterator_t iterator;
  if (piterator == NULL || list == NULL)
    return EINVAL;
  iterator = calloc (sizeof (*iterator), 1);
  if (iterator == NULL)
    return ENOMEM;
  iterator->list = list;
  iterator->cur = NULL;
  iterator->next = list->itr;
  list->itr = iterator;
  iterator->is_advanced = 0;
  *piterator = iterator;
  return 0;
}

void
iterator_destroy (iterator_t *piterator)
{
  iterator_t itr, prev;
  
  if (!piterator || !*piterator)
    return;

  for (itr = (*piterator)->list->itr, prev = NULL;
       itr;
       prev = itr, itr = itr->next)
    if (*piterator == itr)
      break;

  if (itr)
    {
      if (prev)
	prev->next = itr->next;
      else
	itr->list->itr = itr->next;
      free(itr);
      *piterator = NULL;
    }
}

int
iterator_first (iterator_t iterator)
{
  iterator->cur = iterator->list->head.next;
  iterator->is_advanced = 0;
  return 0;
}

int
iterator_next (iterator_t iterator)
{
  if (!iterator->is_advanced)
    iterator->cur = iterator->cur->next;
  iterator->is_advanced = 0;
  return 0;
}

int
iterator_current (iterator_t iterator, void **pitem)
{
  if (!iterator->cur)
    return ENOENT;
  *pitem = iterator->cur->item;
  return 0;
}

int
iterator_is_done (iterator_t iterator)
{
  if (iterator == NULL)
    return 1;
  return iterator->cur == NULL || iterator->cur == &iterator->list->head;
}

int
iterator_get_list (iterator_t iterator, list_t *plist)
{
  if (!iterator)
    return EINVAL;
    *plist = iterator->list;
  return NULL;
}

void
iterator_advance (iterator_t iterator, struct list_data *e)
{
  for (; iterator; iterator = iterator->next)
    {
      if (iterator->cur == e)
	{
	  iterator->cur = e->next;
	  iterator->is_advanced++;
	}
    }
}
