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
(observer_add_ref) (observer_t observer)
{
  if (observer == NULL || observer->vtable == NULL
      || observer->vtable->add_ref == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return observer->vtable->add_ref (observer);
}

int
(observer_release) (observer_t observer)
{
  if (observer == NULL || observer->vtable == NULL
      || observer->vtable->release == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return observer->vtable->release (observer);
}

int
(observer_destroy) (observer_t observer)
{
  if (observer == NULL || observer->vtable == NULL
      || observer->vtable->destroy == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return observer->vtable->destroy (observer);
}

int
(observer_action) (observer_t observer, struct event evt)
{
  if (observer == NULL || observer->vtable == NULL
      || observer->vtable->action == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return observer->vtable->action (observer, evt);
}



static int
_dobserver_add_ref (observer_t observer)
{
  struct _dobserver *dobserver = (struct _dobserver *)observer;
  int status;
  monitor_lock (dobserver->lock);
  status = ++dobserver->ref;
  monitor_unlock (dobserver->lock);
  return status;
}

static int
_dobserver_destroy (observer_t observer)
{
  struct _dobserver *dobserver = (struct _dobserver *)observer;
  monitor_destroy (dobserver->lock);
  free (dobserver);
  return 0;
}

static int
_dobserver_release (observer_t observer)
{
  int status;
  struct _dobserver *dobserver = (struct _dobserver *)observer;
  monitor_lock (dobserver->lock);
  status = --dobserver->ref;
  if (status <= 0)
    {
      monitor_unlock (dobserver->lock);
      return _dobserver_destroy (observer);
    }
  monitor_unlock (dobserver->lock);
  return status;
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
  _dobserver_add_ref,
  _dobserver_release,
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

  dobserver->base.vtable = &_dobserver_vtable;
  dobserver->ref = 1;
  dobserver->arg = arg;
  dobserver->action = action;
  monitor_create (&(dobserver->lock));
  *pobserver = &dobserver->base;
  return 0;
}
