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

#include <stdlib.h>

#include <mailutils/sys/iterator.h>
#include <mailutils/error.h>

int
(iterator_release) (iterator_t iterator)
{
  if (iterator == NULL || iterator->vtable == NULL
      || iterator->vtable->release == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return iterator->vtable->release (iterator);
}

int
(iterator_destroy) (iterator_t iterator)
{
  if (iterator == NULL || iterator->vtable == NULL
      || iterator->vtable->destroy == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return iterator->vtable->destroy (iterator);
}

int
(iterator_first) (iterator_t iterator)
{
  if (iterator == NULL || iterator->vtable == NULL
      || iterator->vtable->first == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return iterator->vtable->first (iterator);
}

int
(iterator_next) (iterator_t iterator)
{
  if (iterator == NULL || iterator->vtable == NULL
      || iterator->vtable->next == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return iterator->vtable->next (iterator);
}

int
(iterator_current) (iterator_t iterator, void *pitem)
{
  if (iterator == NULL || iterator->vtable == NULL
      || iterator->vtable->current == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return iterator->vtable->current (iterator, pitem);
}

int
(iterator_is_done) (iterator_t iterator)
{
  if (iterator == NULL || iterator->vtable == NULL
      || iterator->vtable->is_done == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return iterator->vtable->is_done (iterator);
}
