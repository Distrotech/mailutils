/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2004, 2005 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library; if not, write to the
   Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301 USA */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <errno.h>
#include <stdlib.h>

#include <mailutils/list.h>
#include <mailutils/iterator.h>
#include <mailutils/errno.h>
#include <observer0.h>

int
mu_observer_create (mu_observer_t *pobserver, void *owner)
{
  mu_observer_t observer;
  observer = calloc (sizeof (*observer), 1);
  if (observer == NULL)
    return ENOMEM;
  observer->owner = owner;
  *pobserver = observer;
  return 0;
}

void
mu_observer_destroy (mu_observer_t *pobserver, void *owner)
{
  if (pobserver && *pobserver)
    {
      mu_observer_t observer = *pobserver;
      if (observer->owner == owner || observer->flags & MU_OBSERVER_NO_CHECK)
	{
	  if (observer->_destroy)
	    observer->_destroy (observer);
	  free (observer);
	}
      *pobserver = NULL;
    }
}

void *
mu_observer_get_owner (mu_observer_t observer)
{
  return (observer) ? observer->owner : NULL;
}

int
mu_observer_action (mu_observer_t observer, size_t type)
{
  if (observer == NULL)
    return EINVAL;
  if (observer->_action)
    return observer->_action (observer, type);
  return 0;
}

int
mu_observer_set_action (mu_observer_t observer,
		     int (*_action) (mu_observer_t, size_t), void *owner)
{
  if (observer == NULL)
    return EINVAL;
  if (observer->owner != owner)
    return EACCES;
  observer->_action = _action;
  return 0;
}

int
mu_observer_set_destroy (mu_observer_t observer, int (*_destroy) (mu_observer_t),
		      void *owner)
{
  if (observer == NULL)
    return EINVAL;
  if (observer->owner != owner)
    return EACCES;
  observer->_destroy = _destroy;
  return 0;
}

int
mu_observer_set_flags (mu_observer_t observer, int flags)
{
  if (observer == NULL)
    return EINVAL;
  observer->flags |= flags;
  return 0;
}

int
mu_observable_create (mu_observable_t *pobservable, void *owner)
{
  mu_observable_t observable;
  int status;
  if (pobservable == NULL)
    return MU_ERR_OUT_PTR_NULL;
  observable = calloc (sizeof (*observable), 1);
  if (observable == NULL)
    return ENOMEM;
  status = mu_list_create (&(observable->list));
  if (status != 0 )
    {
      free (observable);
      return status;
    }
  observable->owner = owner;
  *pobservable = observable;
  return 0;
}

void
mu_observable_destroy (mu_observable_t *pobservable, void *owner)
{
  mu_iterator_t iterator;
  if (pobservable && *pobservable)
    {
      mu_observable_t observable = *pobservable;
      if (observable->owner == owner)
	{
	  int status = mu_list_get_iterator (observable->list, &iterator);
	  if (status == 0)
	    {
	      event_t event = NULL;
	      for (mu_iterator_first (iterator); !mu_iterator_is_done (iterator);
		   mu_iterator_next (iterator))
		{
		  event = NULL;
		  mu_iterator_current (iterator, (void **)&event);
		  if (event != NULL)
		    {
		      mu_observer_destroy (&(event->observer), NULL);
		      free (event);
		    }
		}
	      mu_iterator_destroy (&iterator);
	    }
	  mu_list_destroy (&((*pobservable)->list));
	  free (*pobservable);
	}
      *pobservable = NULL;
    }
}

void *
mu_observable_get_owner (mu_observable_t observable)
{
  return (observable) ? observable->owner : NULL;
}

int
mu_observable_attach (mu_observable_t observable, size_t type,  mu_observer_t observer)
{
  event_t event;
  if (observable == NULL || observer == NULL)
    return EINVAL;
  event = calloc (1, sizeof (*event));
  if (event == NULL)
    return ENOMEM;
  event->type = type;
  event->observer = observer;
  return mu_list_append (observable->list, event);
}

int
mu_observable_detach (mu_observable_t observable, mu_observer_t observer)
{
  mu_iterator_t iterator;
  int status;
  int found = 0;
  event_t event = NULL;
  if (observable == NULL || observer == NULL)
    return EINVAL;
  status = mu_list_get_iterator (observable->list, &iterator);
  if (status != 0)
    return status;
  for (mu_iterator_first (iterator); !mu_iterator_is_done (iterator);
       mu_iterator_next (iterator))
    {
      event = NULL;
      mu_iterator_current (iterator, (void **)&event);
      if (event && event->observer == observer)
        {
          found = 1;
          break;
        }
    }
  mu_iterator_destroy (&iterator);
  if (found)
    {
      status = mu_list_remove (observable->list, event);
      free (event);
    }
  else
    status = MU_ERR_NOENT;
  return status;
}

int
mu_observable_notify (mu_observable_t observable, int type)
{
  mu_iterator_t iterator;
  event_t event = NULL;
  int status = 0;
  if (observable == NULL)
    return EINVAL;
  status = mu_list_get_iterator (observable->list, &iterator);
  if (status != 0)
    return status;
  for (mu_iterator_first (iterator); !mu_iterator_is_done (iterator);
       mu_iterator_next (iterator))
    {
      event = NULL;
      mu_iterator_current (iterator, (void **)&event);
      if (event && event->type & type)
        {
	  status |= mu_observer_action (event->observer, type);
        }
    }
  mu_iterator_destroy (&iterator);
  return status;
}
