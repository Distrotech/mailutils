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

#include <attribute.h>

#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define MU_ATTRIBUTE_SEEN ((int)1)
#define MU_ATTRIBUTE_ANSWERED (MU_ATTRIBUTE_SEEN << 1)
#define MU_ATTRIBUTE_FLAGGED  (MU_ATTRIBUTE_ANSWERED << 1)
#define MU_ATTRIBUTE_DELETED  (MU_ATTRIBUTE_FLAGGED << 1)
#define MU_ATTRIBUTE_DRAFT    (MU_ATTRIBUTE_DELETED << 1)
#define MU_ATTRIBUTE_RECENT   (MU_ATTRIBUTE_DRAFT << 1)
#define MU_ATTRIBUTE_READ     (MU_ATTRIBUTE_RECENT << 1)

struct _attribute
{
  size_t flag;
  void *owner;
};

int
attribute_init (attribute_t *pattr, void *owner)
{
  attribute_t attr;
  if (pattr == NULL || owner == NULL)
    return EINVAL;
  attr = calloc (1, sizeof(*attr));
  if (attr == NULL)
    return ENOMEM;
  attr->owner = owner;
  *pattr = attr;
  return 0;
}

void
attribute_destroy (attribute_t *pattr, void *owner)
{
  if (pattr && *pattr)
    {
      attribute_t attr = *pattr;

      if (attr->owner == owner)
	free (attr);
      /* loose the link */
      *pattr = NULL;
    }
  return;
}

int
attribute_set_seen (attribute_t attr, void *owner)
{
  if (attr == NULL)
    return EINVAL;
  if (owner == attr->owner)
    {
      attr->flag |= MU_ATTRIBUTE_SEEN;
      return 0;
    }
  return EACCES;
}

int
attribute_set_answered (attribute_t attr, void *owner)
{
  if (attr == NULL)
    return EINVAL;
  if (owner == attr->owner)
    {
      attr->flag |= MU_ATTRIBUTE_ANSWERED;
      return 0;
    }
  return EACCES;
}

int
attribute_set_flagged (attribute_t attr, void *owner)
{
  if (attr == NULL)
    return EINVAL;
  if (owner == attr->owner)
    {
      attr->flag |= MU_ATTRIBUTE_FLAGGED;
      return 0;
    }
  return EACCES;
}

int
attribute_set_read (attribute_t attr, void *owner)
{
  if (attr == NULL)
    return EINVAL;
  if (owner == attr->owner)
    {
      attr->flag |= MU_ATTRIBUTE_READ;
      return 0;
    }
  return EACCES;
}

int
attribute_set_deleted (attribute_t attr, void *owner)
{
  if (attr == NULL)
    return EINVAL;
  if (owner == attr->owner)
    {
      attr->flag |= MU_ATTRIBUTE_DELETED;
      return 0;
    }
  return EACCES;
}

int
attribute_set_draft (attribute_t attr, void *owner)
{
  if (attr == NULL)
    return EINVAL;
  if (owner == attr->owner)
    {
      attr->flag |= MU_ATTRIBUTE_DRAFT;
      return 0;
    }
  return EACCES;
}

int
attribute_set_recent (attribute_t attr, void *owner)
{
  if (attr == NULL)
    return EINVAL;
  if (attr == NULL)
    {
      attr->flag |= MU_ATTRIBUTE_RECENT;
      return 0;
    }
  return EACCES;
}

int
attribute_is_seen (attribute_t attr)
{
  if (attr == NULL)
    return 0;
  return attr->flag & MU_ATTRIBUTE_SEEN;
}

int
attribute_is_answered (attribute_t attr)
{
  if (attr == NULL)
    return 0;
  return attr->flag & MU_ATTRIBUTE_ANSWERED;
}

int
attribute_is_flagged (attribute_t attr)
{
  if (attr == NULL)
    return 0;
  return attr->flag & MU_ATTRIBUTE_FLAGGED;
}

int
attribute_is_read (attribute_t attr)
{
  if (attr == NULL)
    return 0;
  return attr->flag & MU_ATTRIBUTE_READ;
}

int
attribute_is_deleted (attribute_t attr)
{
  if (attr == NULL)
    return 0;
  return attr->flag & MU_ATTRIBUTE_DELETED;
}

int
attribute_is_draft (attribute_t attr)
{
  if (attr == NULL)
    return 0;
  return attr->flag & MU_ATTRIBUTE_DRAFT;
}

int
attribute_is_recent (attribute_t attr)
{
  if (attr == NULL)
    return 0;
  return attr->flag & MU_ATTRIBUTE_RECENT;
}

int
attribute_unset_seen (attribute_t attr, void *owner)
{
  if (attr == NULL)
    return 0;
  if (owner == attr->owner)
    {
      attr->flag &= ~MU_ATTRIBUTE_SEEN;
      return 0;
    }
  return EACCES;
}

int
attribute_unset_answered (attribute_t attr, void *owner)
{
  if (attr == NULL)
    return 0;
  if (owner == attr->owner)
    {
      attr->flag &= ~MU_ATTRIBUTE_ANSWERED;
      return 0;
    }
  return EACCES;
}

int
attribute_unset_flagged (attribute_t attr, void *owner)
{
  if (attr == NULL)
    return 0;
  if (owner == attr->owner)
    {
      attr->flag &= ~MU_ATTRIBUTE_FLAGGED;
      return 0;
    }
  return EACCES;
}

int
attribute_unset_read (attribute_t attr, void *owner)
{
  if (attr == NULL)
    return 0;
  if (owner == attr->owner)
    {
      attr->flag &= ~MU_ATTRIBUTE_READ;
      return 0;
    }
  return EACCES;
}

int
attribute_unset_deleted (attribute_t attr, void *owner)
{
  if (attr == NULL)
    return 0;
  if (owner == attr->owner)
    {
      attr->flag &= ~MU_ATTRIBUTE_DELETED;
      return 0;
    }
  return EACCES;
}

int
attribute_unset_draft (attribute_t attr, void *owner)
{
  if (attr == NULL)
    return 0;
  if (owner == attr->owner)
    {
      attr->flag &= ~MU_ATTRIBUTE_DRAFT;
      return 0;
    }
  return EACCES;
}

int
attribute_unset_recent (attribute_t attr, void *owner)
{
  if (attr == NULL)
    return 0;
  if (owner == attr->owner)
    {
      attr->flag &= ~MU_ATTRIBUTE_RECENT;
      return 0;
    }
  return EACCES;
}

int
attribute_is_equal (attribute_t attr, attribute_t attr2)
{
  if (attr == NULL || attr2 == NULL)
    return 0;
  return attr->flag == attr2->flag;
}

int
attribute_copy (attribute_t dest, attribute_t src, void *dest_owner)
{
  if (dest == NULL || src == NULL)
    return EINVAL;
  if (dest->owner == dest_owner)
    {
      memcpy (dest, src, sizeof (*dest));
      return 0;
    }
  return EACCES;
}

int
string_to_attribute (const char *buffer, size_t len,
		     attribute_t *pattr, void *owner)
{
  char *sep;
  int status;

  status = attribute_init (pattr, owner);
  if (status != 0)
    return status;

  /* Set the attribute */
  if (len > 7 && strncasecmp (buffer, "Status:", 7) == 0)
    {
      sep = strchr(buffer, ':'); /* pass the ':' */
      if (strchr (sep, 'R') != NULL || strchr (sep, 'r') != NULL)
	attribute_set_read (*pattr, owner);
      if (strchr (sep, 'O') != NULL || strchr (sep, 'o') != NULL)
	attribute_set_seen (*pattr, owner);
      if (strchr (sep, 'A') != NULL || strchr (sep, 'a') != NULL)
	attribute_set_answered (*pattr, owner);
      if (strchr (sep, 'F') != NULL || strchr (sep, 'f') != NULL)
	attribute_set_flagged (*pattr, owner);
    }
  return 0;
}

