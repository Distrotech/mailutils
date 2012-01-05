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

struct map_closure
{
  mu_list_mapper_t map;
  void *data;
  mu_list_t list;
  int status;
};

static int
_list_mapper (void **itmv, size_t itmc, void *data)
{
  struct map_closure *clos = data;
  int rc, status;
      
  if (!clos->list)
    {
      status = mu_list_create (&clos->list);
      if (status)
	{
	  clos->status = status;
	  return MU_ERR_FAILURE;
	}
    }
  
  rc = clos->map (itmv, itmc, clos->data);
  if (!(rc & MU_LIST_MAP_SKIP))
    {
      status = mu_list_append (clos->list, itmv[0]);
      if (status)
	{
	  clos->status = status;
	  return MU_ERR_FAILURE;
	}
    }
  if (rc & MU_LIST_MAP_STOP)
    return MU_ERR_CANCELED;
  return 0;
}
		
int
mu_list_map (mu_list_t list, mu_list_mapper_t map, void *data, size_t nelem,
	     mu_list_t *res)
{
  int rc;
  struct map_closure cl;

  if (!res)
    return MU_ERR_OUT_PTR_NULL;

  cl.map = map;
  cl.data = data;
  cl.list = NULL;
  cl.status = 0;

  rc = mu_list_gmap (list, _list_mapper, nelem, &cl);

  if (cl.list)
    *res = cl.list;
  if (rc == MU_ERR_FAILURE)
    return cl.status;
  return 0;
}
