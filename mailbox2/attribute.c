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

#include <sys/types.h>
#include <stdlib.h>

#include <mailutils/error.h>
#include <mailutils/sys/attribute.h>

int
attribute_ref (attribute_t attribute)
{
  if (attribute == NULL || attribute->vtable == NULL
      || attribute->vtable->ref == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return attribute->vtable->ref (attribute);
}

void
attribute_destroy (attribute_t *pattribute)
{
  if (pattribute && *pattribute)
    {
      attribute_t attribute = *pattribute;
      if (attribute->vtable && attribute->vtable->destroy)
	attribute->vtable->destroy (pattribute);
      *pattribute = NULL;
    }
}

int
attribute_get_flags (attribute_t attribute, int *pflags)
{
  if (attribute == NULL || attribute->vtable == NULL
      || attribute->vtable->get_flags == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return attribute->vtable->get_flags (attribute, pflags);
}

int
attribute_set_flags (attribute_t attribute, int flags)
{
  if (attribute == NULL || attribute->vtable == NULL
      || attribute->vtable->set_flags == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return attribute->vtable->set_flags (attribute, flags);
}

int
attribute_unset_flags (attribute_t attribute, int flags)
{
  if (attribute == NULL || attribute->vtable == NULL
      || attribute->vtable->unset_flags == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return attribute->vtable->unset_flags (attribute, flags);
}

int
attribute_clear_flags (attribute_t attribute)
{
  if (attribute == NULL || attribute->vtable == NULL
      || attribute->vtable->clear_flags == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return attribute->vtable->clear_flags (attribute);
}

int
attribute_get_userflags (attribute_t attribute, int *puserflags)
{
  if (attribute == NULL || attribute->vtable == NULL
      || attribute->vtable->get_userflags == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return attribute->vtable->get_userflags (attribute, puserflags);
}

int
attribute_set_userflags (attribute_t attribute, int userflags)
{
  if (attribute == NULL || attribute->vtable == NULL
      || attribute->vtable->set_userflags == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return attribute->vtable->set_userflags (attribute, userflags);
}

int
attribute_unset_userflags (attribute_t attribute, int userflags)
{
  if (attribute == NULL || attribute->vtable == NULL
      || attribute->vtable->unset_userflags == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return attribute->vtable->unset_userflags (attribute, userflags);
}

int
attribute_clear_userflags (attribute_t attribute)
{
  if (attribute == NULL || attribute->vtable == NULL
      || attribute->vtable->clear_userflags == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return attribute->vtable->clear_userflags (attribute);
}


/* Stub helpers for the wellknown flags.  */
int
attribute_set_seen (attribute_t attribute)
{
  if (attribute == NULL)
    return MU_ERROR_INVALID_PARAMETER;
  return attribute_set_flags (attribute, MU_ATTRIBUTE_SEEN);
}

int
attribute_set_answered (attribute_t attribute)
{
  if (attribute == NULL)
    return MU_ERROR_INVALID_PARAMETER;
  return attribute_set_flags (attribute, MU_ATTRIBUTE_ANSWERED);
}

int
attribute_set_flagged (attribute_t attribute)
{
  if (attribute == NULL)
    return MU_ERROR_INVALID_PARAMETER;
  return attribute_set_flags (attribute, MU_ATTRIBUTE_FLAGGED);
}

int
attribute_set_read (attribute_t attribute)
{
  if (attribute == NULL)
    return MU_ERROR_INVALID_PARAMETER;
  return attribute_set_flags (attribute, MU_ATTRIBUTE_READ);
}

int
attribute_set_deleted (attribute_t attribute)
{
  if (attribute == NULL)
    return MU_ERROR_INVALID_PARAMETER;
  return attribute_set_flags (attribute, MU_ATTRIBUTE_DELETED);
}

int
attribute_set_draft (attribute_t attribute)
{
  if (attribute == NULL)
    return MU_ERROR_INVALID_PARAMETER;
  return attribute_set_flags (attribute, MU_ATTRIBUTE_DRAFT);
}

int
attribute_set_recent (attribute_t attribute)
{
  if (attribute == NULL)
    return MU_ERROR_INVALID_PARAMETER;
  attribute_unset_flags (attribute, MU_ATTRIBUTE_READ);
  return attribute_unset_flags (attribute, MU_ATTRIBUTE_SEEN);
}

int
attribute_set_modified (attribute_t attribute)
{
  if (attribute == NULL)
    return MU_ERROR_INVALID_PARAMETER;
  return attribute_set_flags (attribute, MU_ATTRIBUTE_MODIFIED);
}

int
attribute_is_userflags (attribute_t attribute, int userflag)
{
  int flags = 0;
  if (attribute == NULL)
    return 0;
  attribute_get_userflags (attribute, &flags);
  return flags & userflag;
}

int
attribute_is_seen (attribute_t attribute)
{
  int flags = 0;
  if (attribute == NULL)
    return 0;
  attribute_get_flags (attribute, &flags);
  return flags & MU_ATTRIBUTE_SEEN;
}

int
attribute_is_answered (attribute_t attribute)
{
  int flags = 0;
  if (attribute == NULL)
    return 0;
  attribute_get_flags (attribute, &flags);
  return flags & MU_ATTRIBUTE_ANSWERED;
}

int
attribute_is_flagged (attribute_t attribute)
{
  int flags = 0;
  if (attribute == NULL)
    return 0;
  attribute_get_flags (attribute, &flags);
  return flags & MU_ATTRIBUTE_FLAGGED;
}

int
attribute_is_read (attribute_t attribute)
{
  int flags = 0;
  if (attribute == NULL)
    return 0;
  attribute_get_flags (attribute, &flags);
  return flags & MU_ATTRIBUTE_READ;
}

int
attribute_is_deleted (attribute_t attribute)
{
  int flags = 0;
  if (attribute == NULL)
    return 0;
  attribute_get_flags (attribute, &flags);
  return flags & MU_ATTRIBUTE_DELETED;
}

int
attribute_is_draft (attribute_t attribute)
{
  int flags = 0;
  if (attribute == NULL)
    return 0;
  attribute_get_flags (attribute, &flags);
  return flags & MU_ATTRIBUTE_DRAFT;
}

int
attribute_is_recent (attribute_t attribute)
{
  int flags = 0;
  if (attribute == NULL)
    return 0;
  attribute_get_flags (attribute, &flags);
  /* Something is recent when it is not read and not seen.  */
  return (flags == 0
	  || ! ((flags & MU_ATTRIBUTE_SEEN) || (flags & MU_ATTRIBUTE_READ)));
}

int
attribute_is_modified (attribute_t attribute)
{
  int flags = 0;
  if (attribute == NULL)
      return 0;
  attribute_get_flags (attribute, &flags);
  return flags & MU_ATTRIBUTE_MODIFIED;
}

int
attribute_unset_seen (attribute_t attribute)
{
  if (attribute == NULL)
    return 0;
  return attribute_unset_flags (attribute, MU_ATTRIBUTE_SEEN);
}

int
attribute_unset_answered (attribute_t attribute)
{
  if (attribute == NULL)
    return 0;
  return attribute_unset_flags (attribute, MU_ATTRIBUTE_ANSWERED);
}

int
attribute_unset_flagged (attribute_t attribute)
{
  if (attribute == NULL)
    return 0;
  return attribute_unset_flags (attribute, MU_ATTRIBUTE_FLAGGED);
}

int
attribute_unset_read (attribute_t attribute)
{
  if (attribute == NULL)
    return 0;
  return attribute_unset_flags (attribute, MU_ATTRIBUTE_READ);
}

int
attribute_unset_deleted (attribute_t attribute)
{
  if (attribute == NULL)
    return 0;
  return attribute_unset_flags (attribute, MU_ATTRIBUTE_DELETED);
}

int
attribute_unset_draft (attribute_t attribute)
{
  if (attribute == NULL)
    return 0;
  return attribute_unset_flags (attribute, MU_ATTRIBUTE_DRAFT);
}

int
attribute_unset_recent (attribute_t attribute)
{
  if (attribute == NULL)
    return 0;
  return attribute_unset_flags (attribute, MU_ATTRIBUTE_SEEN);
}

int
attribute_unset_modified (attribute_t attribute)
{
  if (attribute == NULL)
    return 0;
  return attribute_unset_flags (attribute, MU_ATTRIBUTE_MODIFIED);
}

/* Miscellaneous.  */
int
attribute_is_equal (attribute_t attribute1, attribute_t attribute2)
{
  int flags1 = 0;
  int flags2 = 0;
  if (attribute1)
    attribute_get_flags (attribute1, &flags1);
  if (attribute2)
    attribute_get_flags (attribute2, &flags2);
  return flags1 == flags2;
}

int
attribute_copy (attribute_t dest, attribute_t src)
{
  int sflags = 0;
  if (dest == NULL || src == NULL)
    return MU_ERROR_INVALID_PARAMETER;
  attribute_get_flags (src, &sflags);
  attribute_clear_flags (dest);
  attribute_set_flags (dest, sflags);
  return 0;
}
