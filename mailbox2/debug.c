/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Library Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with GNU Mailutils; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>

#include <mailutils/sys/debug.h>
#include <mailutils/error.h>
#include <mailutils/debug.h>

int
mu_debug_ref (mu_debug_t debug)
{
  if (debug == NULL || debug->vtable == NULL || debug->vtable->ref == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return debug->vtable->ref (debug);
}

void
mu_debug_destroy (mu_debug_t *pdebug)
{
  if (pdebug && *pdebug)
    {
      mu_debug_t debug = *pdebug;
      if (debug->vtable && debug->vtable->destroy)
	debug->vtable->destroy (pdebug);
      *pdebug = NULL;
    }
}

int
mu_debug_set_level (mu_debug_t debug, unsigned int level)
{
  if (debug == NULL || debug->vtable == NULL
      || debug->vtable->set_level == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return debug->vtable->set_level (debug, level);
}

int
mu_debug_get_level (mu_debug_t debug, unsigned int *plevel)
{
  if (debug == NULL || debug->vtable == NULL
      || debug->vtable->get_level == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return debug->vtable->get_level (debug, plevel);
}

int
mu_debug_print (mu_debug_t debug, unsigned int level, const char *msg)
{
  if (debug == NULL || debug->vtable == NULL || debug->vtable->print == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return debug->vtable->print (debug, level, msg);
}
