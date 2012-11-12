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
#include <mailutils/sys/list.h>
#include <mailutils/errno.h>

int
mu_list_pop (mu_list_t list, void **item)
{
  struct list_data *last, *prev;
  
  if (list == NULL)
    return EINVAL;

  if (list->count == 0)
    return MU_ERR_NOENT;

  last = list->head.prev;
  prev = last->prev;
  mu_iterator_delitem (list->itr, last);
  prev->next = last->next;
  prev->next->prev = prev;
  if (item)
    *item = last->item;
  else
    DESTROY_ITEM (list, last);
  free (last);
  list->count--;
  return 0;
}
