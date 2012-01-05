/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2011-2012 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>. */

#include <config.h>
#include <stdlib.h>
#include <mailutils/types.h>
#include <mailutils/errno.h>
#include <mailutils/list.h>
#include <mailutils/msgset.h>
#include <mailutils/sys/msgset.h>

static int
count_messages (void *item, void *data)
{
  struct mu_msgrange *r = item;
  size_t *count = data;
  *count += r->msg_end - r->msg_beg + 1;
  return 0;
}
  
int
mu_msgset_count (mu_msgset_t mset, size_t *pcount)
{
  int rc;
  size_t count = 0;

  if (!mset)
    return EINVAL;
  if (!pcount)
    return MU_ERR_OUT_PTR_NULL;
  rc = mu_list_foreach (mset->list, count_messages, &count);
  if (rc == 0)
    *pcount = count;
  return rc;
}
