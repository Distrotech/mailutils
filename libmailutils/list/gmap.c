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
#include <mailutils/errno.h>
#include <mailutils/sys/list.h>

int
mu_list_gmap (mu_list_t list, mu_list_mapper_t map, size_t nelem, void *data)
{
  int rc;
  struct list_data *current;
  void **buf;
  size_t i;
  
  if (!list || !map || nelem == 0)
    return EINVAL;
  buf = calloc (nelem, sizeof (buf[0]));
  if (!buf)
    return ENOMEM;

  i = 0;
  for (current = list->head.next; current != &list->head;
       current = current->next)
    {
      buf[i++] = current->item;
      if (i == nelem)
	{
	  i = 0;
	  rc = map (buf, nelem, data);
	  if (rc)
	    break;
	}
    }
  if (rc == 0 && i > 0 && i < nelem)
    rc = map (buf, i, data);
  free (buf);
  return rc;
}
