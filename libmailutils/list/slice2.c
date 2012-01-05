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
#include <mailutils/list.h>
#include <mailutils/sys/list.h>

int
mu_list_slice2_dup (mu_list_t *pdest, mu_list_t list,
		    size_t from, size_t to,
		    int (*dup_item) (void **, void *, void *),
		    void *dup_data)
{
  size_t arr[2] = { from, to };
  return mu_list_slice_dup (pdest, list, arr, 2, dup_item, dup_data);
}

int
mu_list_slice2 (mu_list_t *pdest, mu_list_t list, size_t from, size_t to)
{
  return mu_list_slice2_dup (pdest, list, from, to, NULL, NULL);
}


  
