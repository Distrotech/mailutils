/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>

#include <mailutils/monitor.h>
#include <mailutils/error.h>
#include <mailutils/sys/refcount.h>

int
mu_refcount_create (mu_refcount_t *prefcount)
{
  mu_refcount_t refcount;
  if (prefcount == NULL)
    return MU_ERROR_INVALID_PARAMETER;
  refcount = calloc (1, sizeof *refcount);
  if (refcount == NULL)
    return MU_ERROR_NO_MEMORY;
  refcount->ref = 1;
  monitor_create (&(refcount->lock));
  *prefcount = refcount;
  return 0;
}

void
mu_refcount_destroy (mu_refcount_t *prefcount)
{
  if (prefcount && *prefcount)
    {
      mu_refcount_t refcount = *prefcount;
      monitor_destroy (&refcount->lock);
      free (refcount);
      *prefcount = NULL;
    }
}

int
mu_refcount_lock (mu_refcount_t refcount)
{
  if (refcount)
    return monitor_lock (refcount->lock);
  return 0;
}

int
mu_refcount_unlock (mu_refcount_t refcount)
{
  if (refcount)
    return monitor_unlock (refcount->lock);
  return 0;
}

int
mu_refcount_inc (mu_refcount_t refcount)
{
  int count = 0;
  if (refcount)
    {
      monitor_lock (refcount->lock);
      count = ++refcount->ref;
      monitor_unlock (refcount->lock);
    }
  return count;
}

int
mu_refcount_dec (mu_refcount_t refcount)
{
  int count = 0;
  if (refcount)
    {
      monitor_lock (refcount->lock);
      if (refcount->ref)
	count = --refcount->ref;
      monitor_unlock (refcount->lock);
    }
  return count;
}
