/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2004, 2005, 2007, 2008, 2010, 2011
   Free Software Foundation, Inc.

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
  int status;
  mu_iterator_t itr;
  void **buf;
  size_t i;
  
  if (!list || !map || nelem == 0)
    return EINVAL;
  rc = mu_list_get_iterator (list, &itr);
  if (rc)
    return status;
  buf = calloc (nelem, sizeof (buf[0]));
  if (!buf)
    {
      mu_iterator_destroy (&itr);
      return ENOMEM;
    }
  for (i = 0, mu_iterator_first (itr); !mu_iterator_is_done (itr);
       mu_iterator_next (itr))
    {
      void *item;
      mu_iterator_current (itr, &item);
      buf[i++] = item;
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
  mu_iterator_destroy (&itr);
  free (buf);
  return rc;
}
