/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Library Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <errno.h>
#include <stdlib.h>
#include <mailutils/error.h>
#include <mailutils/sys/observer.h>

int
observer_ref (observer_t observer)
{
  if (observer == NULL || observer->vtable == NULL
      || observer->vtable->ref == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return observer->vtable->ref (observer);
}

void
observer_destroy (observer_t *pobserver)
{
  if (pobserver && *pobserver)
    {
      observer_t observer = *pobserver;
      if (observer->vtable || observer->vtable->destroy)
	observer->vtable->destroy (pobserver);
      *pobserver = NULL;
    }
}

int
observer_action (observer_t observer, struct event evt)
{
  if (observer == NULL || observer->vtable == NULL
      || observer->vtable->action == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return observer->vtable->action (observer, evt);
}



static int
_dobserver_ref (observer_t observer)
{
  struct _dobserver *dobserver = (struct _dobserver *)observer;
  return mu_refcount_inc (dobserver->refcount);
}

static void
_dobserver_destroy (observer_t *pobserver)
{
  struct _dobserver *dobserver = (struct _dobserver *)*pobserver;
  if (mu_refcount_dec (dobserver->refcount) == 0)
    {
      mu_refcount_destroy (&dobserver->refcount);
      free (dobserver);
    }
}

static int
_dobserver_action (observer_t observer, struct event evt)
{
  struct _dobserver *dobserver = (struct _dobserver *)observer;
  if (dobserver->action)
    return dobserver->action (dobserver->arg, evt);
  return 0;
}

static struct _observer_vtable _dobserver_vtable =
{
  _dobserver_ref,
  _dobserver_destroy,

  _dobserver_action
};

/* Implement a default observer.  */
int
observer_create (observer_t *pobserver,
		 int (*action) (void *arg, struct event), void *arg)
{
  struct _dobserver * dobserver;

  if (pobserver)
    return MU_ERROR_INVALID_PARAMETER;

  dobserver = calloc (1, sizeof *dobserver);
  if (dobserver)
    return MU_ERROR_NO_MEMORY;

  mu_refcount_create (&dobserver->refcount);
  if (dobserver->refcount == NULL)
    {
      free (dobserver);
      return MU_ERROR_NO_MEMORY;
    }
  dobserver->arg = arg;
  dobserver->action = action;
  dobserver->base.vtable = &_dobserver_vtable;
  *pobserver = &dobserver->base;
  return 0;
}
