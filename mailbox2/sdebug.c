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
#include <stdio.h>
#include <string.h>

#include <mailutils/sys/sdebug.h>
#include <mailutils/error.h>

static int
_sdebug_ref (mu_debug_t debug)
{
  struct _sdebug *sdebug = (struct _sdebug *)debug;
  return mu_refcount_inc (sdebug->refcount);
}

static void
_sdebug_destroy (mu_debug_t *pdebug)
{
  if (pdebug && *pdebug)
    {
      struct _sdebug *sdebug = (struct _sdebug *)*pdebug;
      if (mu_refcount_dec (sdebug->refcount) == 0)
	{
	  if (sdebug->stream)
	    {
	      if (sdebug->close_on_destroy)
		stream_close (sdebug->stream);
	      stream_destroy (&sdebug->stream);
	    }
	  mu_refcount_destroy (&sdebug->refcount);
	  free (sdebug);
	}
      *pdebug = NULL;
    }
}

static int
_sdebug_set_level (mu_debug_t debug, size_t level)
{
  struct _sdebug *sdebug = (struct _sdebug *)debug;
  sdebug->level = level;
  return 0;
}

static int
_sdebug_get_level (mu_debug_t debug, size_t *plevel)
{
  struct _sdebug *sdebug = (struct _sdebug *)debug;
  if (plevel)
    *plevel = sdebug->level;
  return 0;
}

static int
_sdebug_print (mu_debug_t debug, size_t level, const char *mesg)
{
  struct _sdebug *sdebug = (struct _sdebug *)debug;

  if (mesg == NULL)
    return MU_ERROR_INVALID_PARAMETER;

  if (!(sdebug->level & level))
    return 0;

  return stream_write (sdebug->stream, mesg, strlen (mesg), NULL);
}

struct _mu_debug_vtable _sdebug_vtable =
{
  _sdebug_ref,
  _sdebug_destroy,

  _sdebug_get_level,
  _sdebug_set_level,
  _sdebug_print
};

int
mu_debug_stream_create (mu_debug_t *pdebug, stream_t stream,
			int close_on_destroy)
{
  struct _sdebug *sdebug;

  if (pdebug == NULL || stream == NULL)
    return MU_ERROR_INVALID_PARAMETER;

  sdebug = calloc (sizeof (*sdebug), 1);
  if (sdebug == NULL)
    return MU_ERROR_NO_MEMORY;

  mu_refcount_create (&sdebug->refcount);
  if (sdebug->refcount == NULL)
    {
      free (sdebug);
      return MU_ERROR_NO_MEMORY;
    }

  sdebug->level = 0;
  sdebug->stream = stream;
  sdebug->close_on_destroy = close_on_destroy;
  sdebug->base.vtable = &_sdebug_vtable;
  *pdebug = &sdebug->base;
  return 0;
}
