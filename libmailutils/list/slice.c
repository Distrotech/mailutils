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

struct slice_info
{
  size_t cur;
  size_t *posv;
  size_t posc;
  size_t idx;
  int (*dup_item) (void **, void *, void *);
  void *dup_data;
  int err;
};

static int
_slice_mapper (void **itmv, size_t itmc, void *call_data)
{
  struct slice_info *si = call_data;
  size_t cur = si->cur++;

  while (1)
    {
      if (cur < si->posv[si->idx])
	return MU_LIST_MAP_SKIP;
      if (si->idx + 1 < si->posc && cur > si->posv[si->idx + 1])
	{
	  si->idx += 2;
	  if (si->idx >= si->posc)
	    return MU_LIST_MAP_STOP|MU_LIST_MAP_SKIP;
	}
      else
	break;
    }
  
  if (si->dup_item && itmv[0])
    {
      void *p;
      int rc = si->dup_item (&p, itmv[0], si->dup_data);
      if (rc)
	{
	  si->err = rc;
	  return MU_LIST_MAP_STOP|MU_LIST_MAP_SKIP;
	}
      itmv[0] = p;
    }
  return MU_LIST_MAP_OK;
}

static int
poscmp (const void *a, const void *b)
{
  size_t posa = *(size_t*)a;
  size_t posb = *(size_t*)b;

  if (posa < posb)
    return -1;
  else if (posa > posb)
    return 1;
  return 0;
}

int
mu_list_slice_dup (mu_list_t *pdest, mu_list_t list, size_t *posv, size_t posc,
		   int (*dup_item) (void **, void *, void *),
		   void *dup_data)
{
  int rc;
  struct slice_info si;
  mu_list_t dest;
  
  si.cur = 0;
  si.idx = 0;
  si.posv = posv;
  si.posc = posc;
  si.dup_item = dup_item;
  si.dup_data = dup_data;
  si.err = 0;

  qsort (posv, posc, sizeof (posv[0]), poscmp);
  
  rc = mu_list_map (list, _slice_mapper, &si, 1, &dest);

  if (rc == 0)
    {
      if (si.err)
	{
	  mu_list_destroy (&dest);
	  rc = si.err;
	}
      else
	{
	  if (dup_item)
	    mu_list_set_destroy_item (dest, list->destroy_item);
	  *pdest = dest;
	}
    }
  return rc;
}

int
mu_list_slice (mu_list_t *pdest, mu_list_t list, size_t *posv, size_t posc)
{
  return mu_list_slice_dup (pdest, list, posv, posc, NULL, NULL);
}
