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
#include <mailutils/errno.h>
#include <mailutils/sys/list.h>

int
mu_list_fold (mu_list_t list, mu_list_folder_t fold, void *data, void *prev,
	      void *return_value)
{
  struct list_data *current;
  int status = 0;
  
  if (list == NULL || fold == NULL)
    return EINVAL;
  if (return_value == NULL)
    return MU_ERR_OUT_PTR_NULL;

  for (current = list->head.next; current != &list->head;
       current = current->next)
    {
      status = fold (current->item, data, prev, &prev);
      if (status)
	break;
    }
  *(void **) return_value = prev;
  return status;
}
