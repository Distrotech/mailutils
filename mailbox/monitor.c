/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000 Free Software Foundation, Inc.

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

#include <errno.h>
#include <stdlib.h>
#include <monitor0.h>

int
monitor_create (monitor_t *pmonitor, void *owner)
{
  monitor_t monitor;
  int status = 0;
  if (pmonitor == NULL)
    return EINVAL;
  monitor = calloc (1, sizeof (*monitor));
  if (monitor == NULL)
    return ENOMEM;
  monitor->owner = owner;
  status = RWLOCK_INIT (&(monitor->lock), NULL);
  if (status != 0)
    {
      free (monitor);
      return status;
    }
  *pmonitor = monitor;
  return status;
}

void *
monitor_get_owner (monitor_t monitor)
{
  return (monitor == NULL) ? NULL : monitor->owner;
}

void
monitor_destroy (monitor_t *pmonitor, void *owner)
{
  if (pmonitor && *pmonitor)
    {
      monitor_t monitor = *pmonitor;
      if (monitor->owner == owner)
	{
	  RWLOCK_DESTROY (&(monitor->lock));
	}
      free (monitor);
      *pmonitor = NULL;
    }
}

int
monitor_rdlock (monitor_t monitor)
{
  if (monitor)
    {
      return RWLOCK_RDLOCK (&(monitor->lock));
    }
  return 0;
}

int
monitor_wrlock  (monitor_t monitor)
{
  if (monitor)
    {
      return RWLOCK_WRLOCK (&(monitor->lock));
    }
  return 0;
}

int
monitor_unlock (monitor_t monitor)
{
  if (monitor)
    {
      return RWLOCK_UNLOCK (&(monitor->lock));
    }
  return 0;
}

int
monitor_wait (monitor_t monitor)
{
  (void)monitor;
  return ENOSYS;
}

int
monitor_notify (monitor_t monitor)
{
  (void)monitor;
  return ENOSYS;
}
