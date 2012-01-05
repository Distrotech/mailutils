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

int
mu_msgset_add_range (mu_msgset_t mset, size_t beg, size_t end, int mode)
{
  int rc;
  struct mu_msgrange *range;
  
  if (!mset || beg == 0)
    return EINVAL;
  if (end && beg > end)
    {
      size_t t = end;
      end = beg;
      beg = t;
    }
  range = calloc (1, sizeof (*range));
  if (!range)
    return ENOMEM;
  range->msg_beg = beg;
  range->msg_end = end;
  rc = _mu_msgset_translate_range (mset, mode, range);
  if (rc)
    {
      free (range);
      return rc;
    }
  rc = mu_list_append (mset->list, range);
  if (rc)
    free (range);
  mset->flags &= ~_MU_MSGSET_AGGREGATED;
  return rc;
}
