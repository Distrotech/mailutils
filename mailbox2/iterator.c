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

#include <stdlib.h>

#include <mailutils/sys/iterator.h>
#include <mailutils/error.h>

void
iterator_destroy (iterator_t *piterator)
{
  if (piterator && *piterator)
    {
      iterator_t iterator = *piterator;
      if (iterator->vtable && iterator->vtable->destroy)
	iterator->vtable->destroy (piterator);
      *piterator = NULL;
    }
}

int
iterator_first (iterator_t iterator)
{
  if (iterator == NULL || iterator->vtable == NULL
      || iterator->vtable->first == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return iterator->vtable->first (iterator);
}

int
iterator_next (iterator_t iterator)
{
  if (iterator == NULL || iterator->vtable == NULL
      || iterator->vtable->next == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return iterator->vtable->next (iterator);
}

int
iterator_current (iterator_t iterator, void *pitem)
{
  if (iterator == NULL || iterator->vtable == NULL
      || iterator->vtable->current == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return iterator->vtable->current (iterator, pitem);
}

int
iterator_is_done (iterator_t iterator)
{
  if (iterator == NULL || iterator->vtable == NULL
      || iterator->vtable->is_done == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return iterator->vtable->is_done (iterator);
}
