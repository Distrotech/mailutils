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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <debug0.h>

int
debug_create (debug_t *pdebug, void *owner)
{
  debug_t debug;
  if (pdebug == NULL)
    return EINVAL;
  debug = calloc (sizeof (*debug), 1);
  if (debug == NULL)
    return ENOMEM;
  debug->owner = owner;
  *pdebug = debug;
  return 0;
}

void
debug_destroy (debug_t *pdebug, void *owner)
{
  if (pdebug && *pdebug)
    {
      debug_t debug = *pdebug;
      if (debug->owner == owner)
	{
	  free (*pdebug);
	  *pdebug = NULL;
	}
    }
}

void *
debug_get_owner (debug_t debug)
{
  return (debug) ? debug->owner : NULL;
}

int
debug_set_level (debug_t debug, size_t level)
{
  if (debug == NULL)
    return EINVAL;
  debug->level = level;
  return 0;
}

int
debug_get_level (debug_t debug, size_t *plevel)
{
  if (debug == NULL)
    return EINVAL;
  if (plevel)
    *plevel = debug->level;
  return 0;
}

int
debug_set_print (debug_t debug, int (*_print)
		 __P ((debug_t, const char *, va_list)), void *owner)
{
  if (debug == NULL)
    return EINVAL;
  if (debug->owner != owner)
    return EACCES;
  debug->_print = _print;
  return 0;
}

/* FIXME:  We use a fix size, we should use vasprinf or something
   similar to get rid of this arbitrary limitation.  */
int
debug_print (debug_t debug, size_t level, const char *format, ...)
{
  va_list ap;
  if (debug == NULL)
    return EINVAL;

  if (!(debug->level & level))
    return 0;

  va_start (ap, format);
  if (debug->_print)
    debug->_print (debug, format, ap);
  else
    vfprintf (stderr, format, ap);
  va_end (ap);
  return 0;
}
