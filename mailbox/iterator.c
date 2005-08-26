/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2004, 2005 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

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

#include <list0.h>
#include <iterator0.h>
#include <mailutils/errno.h>

int
mu_iterator_create (iterator_t *piterator, void *owner)
{
  iterator_t iterator;
  if (piterator == NULL)
    return MU_ERR_OUT_PTR_NULL;
  if (owner == NULL)
    return EINVAL;
  iterator = calloc (sizeof (*iterator), 1);
  if (iterator == NULL)
    return ENOMEM;
  iterator->owner = owner;
  *piterator = iterator;
  return 0;
}

int
mu_iterator_set_first (iterator_t itr, int (*first) (void *))
{
  if (!itr)
    return EINVAL;
  itr->first = first;
  return 0;
}

int
mu_iterator_set_next (iterator_t itr, int (*next) (void *))
{
  if (!itr)
    return EINVAL;
  itr->next = next;
  return 0;
}

int
mu_iterator_set_getitem (iterator_t itr, int (*getitem) (void *, void **))
{
  if (!itr)
    return EINVAL;
  itr->getitem = getitem;
  return 0;
}

int
mu_iterator_set_finished_p (iterator_t itr, int (*finished_p) (void *))
{
  if (!itr)
    return EINVAL;
  itr->finished_p = finished_p;
  return 0;
}

int
mu_iterator_set_curitem_p (iterator_t itr,
			int (*curitem_p) (void *, void *))
{
  if (!itr)
    return EINVAL;
  itr->curitem_p = curitem_p;
  return 0;
}

int
mu_iterator_set_destroy (iterator_t itr, int (destroy) (iterator_t, void *))
{
  if (!itr)
    return EINVAL;
  itr->destroy = destroy;
  return 0;
}

int
mu_iterator_set_dup (iterator_t itr, int (dup) (void **ptr, void *data))
{
  if (!itr)
    return EINVAL;
  itr->dup = dup;
  return 0;
}



int
mu_iterator_dup (iterator_t *piterator, iterator_t orig)
{
  iterator_t iterator;
  int status;
  
  if (piterator == NULL)
    return MU_ERR_OUT_PTR_NULL;
  if (orig == NULL)
    return EINVAL;

  status = mu_iterator_create (&iterator, orig->owner);
  if (status)
    return status;

  status = orig->dup(&iterator->owner, orig->owner);
  if (status)
    {
      free (iterator);
      return status;
    }
  iterator->is_advanced = orig->is_advanced;   
  iterator->first = orig->first;
  iterator->next = orig->next;
  iterator->getitem = orig->getitem;
  iterator->finished_p = orig->finished_p;
  iterator->curitem_p = orig->curitem_p;
  iterator->dup = orig->dup;
  iterator->destroy = orig->destroy;

  *piterator = iterator;
  return 0;
}

void
mu_iterator_destroy (iterator_t *piterator)
{
  if (!piterator || !*piterator)
    return;

  if ((*piterator)->destroy)
    (*piterator)->destroy (*piterator, (*piterator)->owner);
  
  free (*piterator);
  *piterator = NULL;
}

int
mu_iterator_first (iterator_t iterator)
{
  iterator->is_advanced = 0;
  return iterator->first (iterator->owner);
}

int
mu_iterator_next (iterator_t iterator)
{
  int status = 0;
  if (!iterator->is_advanced)
    status = iterator->next (iterator->owner);
  iterator->is_advanced = 0;
  return status;
}

int
mu_iterator_current (iterator_t iterator, void * const *pitem)
{
  return iterator->getitem (iterator->owner, (void**)pitem);
}

int
mu_iterator_is_done (iterator_t iterator)
{
  if (iterator == NULL)
    return 1;
  return iterator->finished_p (iterator->owner);
}

int
iterator_get_owner (iterator_t iterator, void **powner)
{
  if (!iterator)
    return EINVAL;
  if (!powner)
    return MU_ERR_OUT_PTR_NULL;
  *powner = iterator->owner;
  return 0;
}

void
mu_iterator_advance (iterator_t iterator, void *e)
{
  for (; iterator; iterator = iterator->next_itr)
    {
      if (iterator->curitem_p (iterator->owner, e))
	{
	  iterator->next (iterator->owner);
	  iterator->is_advanced++;
	}
    }
}

int
mu_iterator_attach (iterator_t *root, iterator_t iterator)
{
  iterator->next_itr = *root;
  *root = iterator;
  return 0;
}

int
mu_iterator_detach (iterator_t *root, iterator_t iterator)
{
  iterator_t itr, prev;
  
  for (itr = *root, prev = NULL; itr; prev = itr, itr = itr->next_itr)
    if (iterator == itr)
      break;

  if (itr)
    {
      if (prev)
	prev->next = itr->next;
      else
	*root = itr->next_itr;
    }
  
  return 0;
}
