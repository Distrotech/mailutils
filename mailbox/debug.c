/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2004, 2005, 2007  Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

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
#include <stdio.h>
#include <string.h>

#include <mailutils/errno.h>
#include <debug0.h>

int
mu_debug_create (mu_debug_t *pdebug, void *owner)
{
  mu_debug_t debug;
  if (pdebug == NULL)
    return MU_ERR_OUT_PTR_NULL;
  debug = calloc (sizeof (*debug), 1);
  if (debug == NULL)
    return ENOMEM;
  debug->owner = owner;
  *pdebug = debug;
  return 0;
}

void
mu_debug_destroy (mu_debug_t *pdebug, void *owner)
{
  if (pdebug && *pdebug)
    {
      mu_debug_t debug = *pdebug;
      if (debug->owner == owner)
	{
	  free (*pdebug);
	  *pdebug = NULL;
	}
    }
}

void *
mu_debug_get_owner (mu_debug_t debug)
{
  return (debug) ? debug->owner : NULL;
}

int
mu_debug_set_level (mu_debug_t debug, size_t level)
{
  if (debug == NULL)
    return EINVAL;
  debug->level = level;
  return 0;
}

int
mu_debug_get_level (mu_debug_t debug, size_t *plevel)
{
  if (debug == NULL)
    return EINVAL;
  if (plevel)
    *plevel = debug->level;
  return 0;
}

int
mu_debug_set_print (mu_debug_t debug,
    int (*_print) (mu_debug_t, size_t, const char *, va_list),
    void *owner)
{
  if (debug == NULL)
    return EINVAL;
  if (debug->owner != owner)
    return EACCES;
  debug->_print = _print;
  return 0;
}

int
mu_debug_print (mu_debug_t debug, size_t level, const char *format, ...)
{
  va_list ap;

  va_start (ap, format);

  mu_debug_printv (debug, level, format, ap);
  
  va_end (ap);

  return 0;
}

int
mu_debug_printv (mu_debug_t debug, size_t level, const char *format, va_list ap)
{
  if (debug == NULL || format == NULL)
    return EINVAL;

  if (!(debug->level & level))
    return 0;

  if (debug->_print)
    debug->_print (debug, level, format, ap);
  else
    vfprintf (stderr, format, ap);

  return 0;
}

