/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2012 Free Software Foundation, Inc.

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
mu_list_remove_nth (mu_list_t list, size_t n)
{
  struct list_data *current;
  int status = MU_ERR_NOENT;
  size_t i;
    
  if (list == NULL)
    return EINVAL;
  if (n >= list->count)
    return ERANGE;
  mu_monitor_wrlock (list->monitor);
  for (current = list->head.next, i = 0; current != &list->head;
       current = current->next, i++)
    {
      if (i == n)
	{
	  struct list_data *previous = current->prev;
	  
	  mu_iterator_delitem (list->itr, current);
	  previous->next = current->next;
	  current->next->prev = previous;
	  DESTROY_ITEM (list, current);
	  free (current);
	  list->count--;
	  status = 0;
	  break;
	}
    }
  mu_monitor_unlock (list->monitor);
  return status;
}

int
mu_list_remove_nth_nd (mu_list_t list, size_t n)
{
  mu_list_destroy_item_t dptr = mu_list_set_destroy_item (list, NULL);
  int rc = mu_list_remove_nth (list, n);
  mu_list_set_destroy_item (list, dptr);
  return rc;
}

