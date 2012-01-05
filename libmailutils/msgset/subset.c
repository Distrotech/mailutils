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

struct sub_closure
{
  int mode;
  mu_msgset_t dest;
};

static int
sub_range (void *item, void *data)
{
  struct mu_msgrange *r = item;
  struct sub_closure *clos = data;
  return mu_msgset_sub_range (clos->dest, r->msg_beg, r->msg_end, clos->mode);
}

int
mu_msgset_sub (mu_msgset_t a, mu_msgset_t b)
{
  struct sub_closure closure;
  if (!a)
    return EINVAL;
  if (!b)
    return 0;
  closure.mode = b->flags;
  closure.dest = a;
  return mu_list_foreach (b->list, sub_range, &closure);
}
