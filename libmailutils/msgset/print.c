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
#include <mailutils/stream.h>
#include <mailutils/io.h>
#include <mailutils/msgset.h>
#include <mailutils/sys/msgset.h>

struct print_env
{
  mu_stream_t stream;
  int cont;
};

static int
_msgrange_printer (void *item, void *data)
{
  int rc;
  struct mu_msgrange *range = item;
  struct print_env *env = data;

  if (env->cont)
    {
      rc = mu_stream_write (env->stream, ",", 1, NULL);
      if (rc)
	return rc;
    }
  else
    env->cont = 1;
  if (range->msg_beg == range->msg_end)
    rc = mu_stream_printf (env->stream, "%lu", (unsigned long) range->msg_beg);
  else if (range->msg_end == 0)
    rc = mu_stream_printf (env->stream, "%lu:*",
			   (unsigned long) range->msg_beg);
  else
    rc = mu_stream_printf (env->stream, "%lu:%lu",
			   (unsigned long) range->msg_beg,
			   (unsigned long) range->msg_end);
  return rc;
}

int
mu_msgset_print (mu_stream_t str, mu_msgset_t mset)
{
  struct print_env env;
  int rc;
  
  if (mu_list_is_empty (mset->list))
    return MU_ERR_NOENT;
  rc = mu_msgset_aggregate (mset);
  if (rc)
    return rc;
  env.stream = str;
  env.cont = 0;
  return mu_list_foreach (mset->list, _msgrange_printer, &env);
}
