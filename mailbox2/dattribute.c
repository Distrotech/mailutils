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

#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif

#include <mailutils/error.h>
#include <mailutils/sys/attribute.h>

static int
_da_ref (attribute_t attribute)
{
  struct _da *da = (struct _da *)attribute;
  return mu_refcount_inc (da->refcount);
}

static void
_da_destroy (attribute_t *pattribute)
{
  struct _da *da = (struct _da *)*pattribute;
  if (mu_refcount_dec (da->refcount) == 0)
    {
      mu_refcount_destroy (&da->refcount);
      free (da);
    }
}

static int
_da_get_flags (attribute_t attribute, int *pflags)
{
  struct _da *da = (struct _da *)attribute;
  mu_refcount_lock (da->refcount);
  if (pflags)
    *pflags = da->flags;
  mu_refcount_unlock (da->refcount);
  return 0;
}

static int
_da_set_flags (attribute_t attribute, int flags)
{
  struct _da *da = (struct _da *)attribute;
  mu_refcount_lock (da->refcount);
  da->flags |= (flags | MU_ATTRIBUTE_MODIFIED);
  mu_refcount_unlock (da->refcount);
  return 0;
}

static int
_da_unset_flags (attribute_t attribute, int flags)
{
  struct _da *da = (struct _da *)attribute;
  mu_refcount_lock (da->refcount);
  da->flags &= ~flags;
  /* If Modified was being unset do not reset it.  */
  if (!(flags & MU_ATTRIBUTE_MODIFIED))
    da->flags |= MU_ATTRIBUTE_MODIFIED;
  mu_refcount_unlock (da->refcount);
  return 0;
}

static int
_da_clear_flags (attribute_t attribute)
{
  struct _da *da = (struct _da *)attribute;
  mu_refcount_lock (da->refcount);
  da->flags = 0;
  mu_refcount_unlock (da->refcount);
  return 0;
}

static struct _attribute_vtable _da_vtable =
{
  _da_ref,
  _da_destroy,

  _da_get_flags,
  _da_set_flags,
  _da_unset_flags,
  _da_clear_flags
};

int
attribute_create (attribute_t *pattribute)
{
  struct _da *da;
  if (pattribute == NULL)
    return MU_ERROR_INVALID_PARAMETER;
  da = calloc (1, sizeof *da);
  if (da == NULL)
    return MU_ERROR_NO_MEMORY;

  mu_refcount_create (&(da->refcount));
  if (da->refcount == NULL)
    {
      free (da);
      return MU_ERROR_NO_MEMORY;
    }
  da->flags = 0;
  da->base.vtable = &_da_vtable;
  *pattribute = &da->base;
  return 0;
}
