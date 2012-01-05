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
mu_list_get (mu_list_t list, size_t indx, void **pitem)
{
  struct list_data *current;
  size_t count;
  int status = MU_ERR_NOENT;

  if (list == NULL)
    return EINVAL;
  if (pitem == NULL)
    return MU_ERR_OUT_PTR_NULL;
  mu_monitor_rdlock (list->monitor);
  for (current = list->head.next, count = 0; current != &list->head;
       current = current->next, count++)
    {
      if (count == indx)
        {
          *pitem = current->item;
	  status = 0;
	  break;
        }
    }
  mu_monitor_unlock (list->monitor);
  return status;
}
