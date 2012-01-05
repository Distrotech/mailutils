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

/* Computes an intersection of two lists and returns it in PDEST.
   The resulting list contains elements from A that are
   also encountered in B (as per comparison function of
   the latter).

   If DUP_ITEM is not NULL, it is used to create copies of
   items to be stored in PDEST.  In this case, the destroy_item
   function of B is also attached to PDEST.  Otherwise, if
   DUP_ITEM is NULL, pointers to elements are stored and
   no destroy_item function is assigned. */
int
mu_list_intersect_dup (mu_list_t *pdest, mu_list_t a, mu_list_t b,
		       int (*dup_item) (void **, void *, void *),
		       void *dup_closure)
{
  mu_list_t dest;
  int rc;
  mu_iterator_t itr;
  
  rc = mu_list_create (&dest);
  if (rc)
    return rc;

  mu_list_set_comparator (dest, b->comp);
  if (dup_item)
    mu_list_set_destroy_item (dest, b->destroy_item);
  
  rc = mu_list_get_iterator (a, &itr);
  if (rc)
    {
      mu_list_destroy (&dest);
      return rc;
    }

  rc = 0;
  for (mu_iterator_first (itr); !mu_iterator_is_done (itr);
       mu_iterator_next (itr))
    {
      void *data;
      mu_iterator_current (itr, &data);
      if (mu_list_locate (b, data, NULL) == 0)
	{
	  void *new_data;
	  if (dup_item && data)
	    {
	      rc = dup_item (&new_data, data, dup_closure);
	      if (rc)
		break;
	    }
	  else
	    new_data = data;
	
	  mu_list_append (dest, new_data); /* FIXME: Check return, and? */
	}
    }
  mu_iterator_destroy (&itr);
  *pdest = dest;
  return rc;
}

int
mu_list_intersect (mu_list_t *pdest, mu_list_t a, mu_list_t b)
{
  return mu_list_intersect_dup (pdest, a, b, NULL, NULL);
}
