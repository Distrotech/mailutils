/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2001, 2004-2005, 2007-2008, 2010-2012 Free
   Software Foundation, Inc.

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
#include <mailutils/sys/list.h>
#include <mailutils/errno.h>

int
mu_list_to_array (mu_list_t list, void **array, size_t count, size_t *pcount)
{
  size_t total = 0;

  if (!list)
    return EINVAL;

  total = (count < list->count) ? count : list->count;

  if (array)
    {
      size_t i;
      struct list_data *current;

      for (i = 0, current = list->head.next;
	   i < total && current != &list->head; current = current->next)
	array[i++] = current->item;
    }
  if (pcount)
    *pcount = total;
  return 0;
}

