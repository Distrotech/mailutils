/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2004, 2005  Free Software Foundation, Inc.

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
observer_create (observer_t *pobserver, void *owner)
{
  observer_t observer;
  observer = calloc (sizeof (*observer), 1);
  if (observer == NULL)
    return ENOMEM;
  observer->owner = owner;
  *pobserver = observer;
  return 0;
}

void
observer_destroy (observer_t *pobserver, void *owner)
{
  if (pobserver && *pobserver)
    {
      observer_t observer = *pobserver;
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
observer_get_owner (observer_t observer)
{
  return (observer) ? observer->owner : NULL;
}

int
observer_action (observer_t observer, size_t type)
{
  if (observer == NULL)
    return EINVAL;
  if (observer->_action)
    return observer->_action (observer, type);
  return 0;
}

int
observer_set_action (observer_t observer,
		     int (*_action) (observer_t, size_t), void *owner)
{
  if (observer == NULL)
    return EINVAL;
  if (observer->owner != owner)
    return EACCES;
  observer->_action = _action;
  return 0;
}

int
observer_set_destroy (observer_t observer, int (*_destroy) (observer_t),
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
observer_set_flags (observer_t observer, int flags)
{
  if (observer == NULL)
    return EINVAL;
  observer->flags |= flags;
  return 0;
}

int
observable_create (observable_t *pobservable, void *owner)
{
  observable_t observable;
  int status;
  if (pobservable == NULL)
    return MU_ERR_OUT_PTR_NULL;
  observable = calloc (sizeof (*observable), 1);
  if (observable == NULL)
    return ENOMEM;
  status = list_create (&(observable->list));
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
observable_destroy (observable_t *pobservable, void *owner)
{
  iterator_t iterator;
  if (pobservable && *pobservable)
    {
      observable_t observable = *pobservable;
      if (observable->owner == owner)
	{
	  int status = list_get_iterator (observable->list, &iterator);
	  if (status == 0)
	    {
	      event_t event = NULL;
	      for (iterator_first (iterator); !iterator_is_done (iterator);
		   iterator_next (iterator))
		{
		  event = NULL;
		  iterator_current (iterator, (void **)&event);
		  if (event != NULL)
		    {
		      observer_destroy (&(event->observer), NULL);
		      free (event);
		    }
		}
	      iterator_destroy (&iterator);
	    }
	  list_destroy (&((*pobservable)->list));
	  free (*pobservable);
	}
      *pobservable = NULL;
    }
}

void *
observable_get_owner (observable_t observable)
{
  return (observable) ? observable->owner : NULL;
}

int
observable_attach (observable_t observable, size_t type,  observer_t observer)
{
  event_t event;
  if (observable == NULL || observer == NULL)
    return EINVAL;
  event = calloc (1, sizeof (*event));
  if (event == NULL)
    return ENOMEM;
  event->type = type;
  event->observer = observer;
  return list_append (observable->list, event);
}

int
observable_detach (observable_t observable, observer_t observer)
{
  iterator_t iterator;
  int status;
  int found = 0;
  event_t event = NULL;
  if (observable == NULL || observer == NULL)
    return EINVAL;
  status = list_get_iterator (observable->list, &iterator);
  if (status != 0)
    return status;
  for (iterator_first (iterator); !iterator_is_done (iterator);
       iterator_next (iterator))
    {
      event = NULL;
      iterator_current (iterator, (void **)&event);
      if (event && event->observer == observer)
        {
          found = 1;
          break;
        }
    }
  iterator_destroy (&iterator);
  if (found)
    {
      status = list_remove (observable->list, event);
      free (event);
    }
  else
    status = MU_ERR_NOENT;
  return status;
}

int
observable_notify (observable_t observable, int type)
{
  iterator_t iterator;
  event_t event = NULL;
  int status = 0;
  if (observable == NULL)
    return EINVAL;
  status = list_get_iterator (observable->list, &iterator);
  if (status != 0)
    return status;
  for (iterator_first (iterator); !iterator_is_done (iterator);
       iterator_next (iterator))
    {
      event = NULL;
      iterator_current (iterator, (void **)&event);
      if (event && event->type & type)
        {
	  status |= observer_action (event->observer, type);
        }
    }
  iterator_destroy (&iterator);
  return status;
}
