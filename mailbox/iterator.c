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
