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
#include <mailutils/sys/dattribute.h>

int
_attribute_default_ref (attribute_t attribute)
{
  struct _attribute_default *da = (struct _attribute_default *)attribute;
  return mu_refcount_inc (da->refcount);
}

void
_attribute_default_destroy (attribute_t *pattribute)
{
  struct _attribute_default *da = (struct _attribute_default *)*pattribute;
  if (mu_refcount_dec (da->refcount) == 0)
    {
      mu_refcount_destroy (&da->refcount);
      free (da);
    }
}

int
_attribute_default_get_flags (attribute_t attribute, int *pflags)
{
  struct _attribute_default *da = (struct _attribute_default *)attribute;
  mu_refcount_lock (da->refcount);
  if (pflags)
    *pflags = da->flags;
  mu_refcount_unlock (da->refcount);
  return 0;
}

int
_attribute_default_set_flags (attribute_t attribute, int flags)
{
  struct _attribute_default *da = (struct _attribute_default *)attribute;
  mu_refcount_lock (da->refcount);
  da->flags |= (flags | MU_ATTRIBUTE_MODIFIED);
  mu_refcount_unlock (da->refcount);
  return 0;
}

int
_attribute_default_unset_flags (attribute_t attribute, int flags)
{
  struct _attribute_default *da = (struct _attribute_default *)attribute;
  mu_refcount_lock (da->refcount);
  da->flags &= ~flags;
  /* If Modified was being unset do not reset it.  */
  if (!(flags & MU_ATTRIBUTE_MODIFIED))
    da->flags |= MU_ATTRIBUTE_MODIFIED;
  mu_refcount_unlock (da->refcount);
  return 0;
}

int
_attribute_default_clear_flags (attribute_t attribute)
{
  struct _attribute_default *da = (struct _attribute_default *)attribute;
  mu_refcount_lock (da->refcount);
  da->flags = 0;
  mu_refcount_unlock (da->refcount);
  return 0;
}

int
_attribute_default_get_userflags (attribute_t attribute, int *puserflags)
{
  struct _attribute_default *da = (struct _attribute_default *)attribute;
  mu_refcount_lock (da->refcount);
  if (puserflags)
    *puserflags = da->userflags;
  mu_refcount_unlock (da->refcount);
  return 0;
}

int
_attribute_default_set_userflags (attribute_t attribute, int userflags)
{
  struct _attribute_default *da = (struct _attribute_default *)attribute;
  mu_refcount_lock (da->refcount);
  da->userflags |= (userflags | MU_ATTRIBUTE_MODIFIED);
  mu_refcount_unlock (da->refcount);
  return 0;
}

int
_attribute_default_unset_userflags (attribute_t attribute, int userflags)
{
  struct _attribute_default *da = (struct _attribute_default *)attribute;
  mu_refcount_lock (da->refcount);
  da->userflags &= ~userflags;
  /* If Modified was being unset do not reset it.  */
  if (!(userflags & MU_ATTRIBUTE_MODIFIED))
    da->userflags |= MU_ATTRIBUTE_MODIFIED;
  mu_refcount_unlock (da->refcount);
  return 0;
}

int
_attribute_default_clear_userflags (attribute_t attribute)
{
  struct _attribute_default *da = (struct _attribute_default *)attribute;
  mu_refcount_lock (da->refcount);
  da->userflags = 0;
  mu_refcount_unlock (da->refcount);
  return 0;
}

static struct _attribute_vtable _attribute_default_vtable =
{
  _attribute_default_ref,
  _attribute_default_destroy,

  _attribute_default_get_flags,
  _attribute_default_set_flags,
  _attribute_default_unset_flags,
  _attribute_default_clear_flags,

  _attribute_default_get_userflags,
  _attribute_default_set_userflags,
  _attribute_default_unset_userflags,
  _attribute_default_clear_userflags
};

int
_attribute_default_ctor (struct _attribute_default *da)
{
  mu_refcount_create (&(da->refcount));
  if (da->refcount == NULL)
    return MU_ERROR_NO_MEMORY;
  da->flags = 0;
  da->base.vtable = &_attribute_default_vtable;
  return 0;
}

void
_attribute_default_dtor (attribute_t attribute)
{
  struct _attribute_default *da = (struct _attribute_default *)attribute;
  if (da)
    mu_refcount_destroy (&da->refcount);
}

int
attribute_default_create (attribute_t *pattribute)
{
  struct _attribute_default *da;
  int status;

  if (pattribute == NULL)
    return MU_ERROR_INVALID_PARAMETER;

  da = calloc (1, sizeof *da);
  if (da == NULL)
    return MU_ERROR_NO_MEMORY;

  status = _attribute_default_ctor (da);
  if (status != 0)
    {
      free (da);
      return status;
    }
  *pattribute = &da->base;
  return 0;
}

int
attribute_status_create (attribute_t *pattribute, const char *field)
{
  struct _attribute_default *da;
  int status;
  char *colon;

  if (pattribute == NULL || field == NULL)
    return MU_ERROR_INVALID_PARAMETER;

  da = calloc (1, sizeof *da);
  if (da == NULL)
    return MU_ERROR_NO_MEMORY;

  status = _attribute_default_ctor (da);
  if (status != 0)
    {
      free (da);
      return status;
    }
  *pattribute = &da->base;

  colon = strchr (field, ':');
  if (colon)
    field = ++colon;

  for (; *field; field++)
    {
      switch (*field)
	{
	case 'O':
	case 'o':
	  attribute_set_seen (*pattribute);
	  break;

	case 'r':
	case 'R':
	  attribute_set_read (*pattribute);
	  break;

	case 'a':
	case 'A':
	  attribute_set_answered (*pattribute);
	  break;

	case 'd':
	case 'D':
	  attribute_set_deleted (*pattribute);
	  break;

	case 't':
	case 'T':
	  attribute_set_draft (*pattribute);
	  break;

	case 'f':
	case 'F':
	  attribute_set_flagged (*pattribute);
	  break;

	}
    }
  attribute_unset_modified (*pattribute);
  return 0;
}
