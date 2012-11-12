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
#include <stdlib.h>
#include <mailutils/sys/list.h>
#include <mailutils/errno.h>

int
mu_list_remove (mu_list_t list, void *item)
{
  struct list_data *current;
  mu_list_comparator_t comp;
  int status = MU_ERR_NOENT;

  if (list == NULL)
    return EINVAL;
  comp = list->comp ? list->comp : _mu_list_ptr_comparator;
  mu_monitor_wrlock (list->monitor);
  for (current = list->head.next;
       current != &list->head; current = current->next)
    {
      if (comp (current->item, item) == 0)
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
mu_list_remove_nd (mu_list_t list, void *item)
{
  mu_list_destroy_item_t dptr = mu_list_set_destroy_item (list, NULL);
  int rc = mu_list_remove (list, item);
  mu_list_set_destroy_item (list, dptr);
  return rc;
}

