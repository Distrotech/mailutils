/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2003, 2004, 
   2007 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

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
#include <errno.h>

#include <mailutils/monitor.h>
#include <mailutils/refcount.h>

int
mu_refcount_create (mu_refcount_t *prefcount)
{
  int status = 0;
  mu_refcount_t refcount;
  if (prefcount == NULL)
    return MU_ERR_OUT_PTR_NULL;
  refcount = calloc (1, sizeof *refcount);
  if (refcount != NULL)
    {
      refcount->ref = 1;
      status = monitor_create (&(refcount->lock));
      if (status == 0)
	{
	  *prefcount = refcount;
	}
      else
	{
	  free (refcount);
	}
    }
  else
    {
      status = ENOMEM;
    }
  return status;
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
