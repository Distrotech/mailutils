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

#include <mailutils/list.h>

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
  *piterator = iterator;
  return 0;
}

void
iterator_destroy (iterator_t *piterator)
{
  if (piterator && *piterator)
    {
      free (*piterator);
      *piterator = NULL;
    }
}

int
iterator_first (iterator_t iterator)
{
  iterator->index = 0;
  return 0;
}

int
iterator_next (iterator_t iterator)
{
  iterator->index++;
  return 0;
}

int
iterator_current (iterator_t iterator, void **pitem)
{
  return list_get (iterator->list, iterator->index, pitem);
}

int
iterator_is_done (iterator_t iterator)
{
  size_t count;
  int status;
  if (iterator == NULL)
    return 1;
  status = list_count (iterator->list, &count);
  if (status != 0)
    return 1;
  return (iterator->index >= count);
}
