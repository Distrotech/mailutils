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
#include <mailutils/iterator.h>
#include <mailutils/error.h>
#include <mailutils/sys/observer.h>

struct observer_info
{
  int type;
  observer_t observer;
};

int
observable_create (observable_t *pobservable)
{
  observable_t observable;
  int status;
  if (pobservable == NULL)
    return MU_ERROR_INVALID_PARAMETER;
  observable = calloc (sizeof (*observable), 1);
  if (observable == NULL)
    return MU_ERROR_NO_MEMORY;
  status = list_create (&(observable->list));
  if (status != 0 )
    {
      free (observable);
      return status;
    }
  *pobservable = observable;
  return 0;
}

int
observable_release (observable_t observable)
{
  (void)observable;
  return 1;
}

int
observable_destroy (observable_t observable)
{
  if (observable)
    {
      iterator_t iterator = NULL;
      int status = list_get_iterator (observable->list, &iterator);
      if (status == 0)
	{
	  struct observer_info *info;
	  for (iterator_first (iterator); !iterator_is_done (iterator);
	       iterator_next (iterator))
	    {
	      info = NULL;
	      iterator_current (iterator, (void **)&info);
	      if (info)
		{
		  observer_release (info->observer);
		  free (info);
		}
	    }
	  iterator_release (iterator);
	}
      list_destroy (observable->list);
      free (observable);
    }
  return 0;
}

int
observable_attach (observable_t observable, int type,  observer_t observer)
{
  struct observer_info *info;
  if (observable == NULL || observer == NULL)
    return MU_ERROR_INVALID_PARAMETER;
  info = calloc (1, sizeof *info);
  if (info == NULL)
    return MU_ERROR_NO_MEMORY;
  info->type = type;
  info->observer = observer;
  return list_append (observable->list, info);
}

int
observable_detach (observable_t observable, observer_t observer)
{
  iterator_t iterator = NULL;
  int status;
  int found = 0;
  struct observer_info *info;
  if (observable == NULL ||observer == NULL)
    return EINVAL;
  status = list_get_iterator (observable->list, &iterator);
  if (status != 0)
    return status;
  for (iterator_first (iterator); !iterator_is_done (iterator);
       iterator_next (iterator))
    {
      info = NULL;
      iterator_current (iterator, (void **)&info);
      if (info && (int)(info->observer) == (int)observer)
        {
          found = 1;
          break;
        }
    }
  iterator_release (iterator);
  if (found)
    {
      status = list_remove (observable->list, info);
      free (info);
    }
  return status;
}

int
observable_notify_all (observable_t observable, struct event evt)
{
  iterator_t iterator;
  struct observer_info *info;
  int status = 0;

  if (observable == NULL)
    return MU_ERROR_INVALID_PARAMETER;
  status = list_get_iterator (observable->list, &iterator);
  if (status != 0)
    return status;
  for (iterator_first (iterator); !iterator_is_done (iterator);
       iterator_next (iterator))
    {
      info = NULL;
      iterator_current (iterator, (void **)&info);
      if (info && info->type & evt.type)
        {
	  status |= observer_action (info->observer, evt);
        }
    }
  iterator_release (iterator);
  return status;
}
