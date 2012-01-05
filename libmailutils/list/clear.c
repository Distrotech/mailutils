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

void
mu_list_clear (mu_list_t list)
{
  struct list_data *current;
  struct list_data *previous;

  if (!list)
    return;
  mu_monitor_wrlock (list->monitor);
  for (current = list->head.next; current != &list->head;)
    {
      previous = current;
      current = current->next;
      DESTROY_ITEM (list, previous);
      free (previous);
    }
  list->head.next = list->head.prev = &list->head;
  mu_monitor_unlock (list->monitor);
}
