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

#include <attribute0.h>

static int flags_to_string __P ((int, char *, size_t, size_t *));

int
attribute_create (attribute_t *pattr, void *owner)
{
  attribute_t attr;
  if (pattr == NULL)
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
	free (*pattr);
      /* loose the link */
      *pattr = NULL;
    }
}

void *
attribute_get_owner (attribute_t attr)
{
  return (attr) ? attr->owner : NULL;
}

int
attribute_is_modified (attribute_t attr)
{
  return (attr) ? attr->flags & MU_ATTRIBUTE_MODIFIED : 0;
}

int
attribute_clear_modified (attribute_t attr)
{
  if (attr)
      attr->flags &= ~MU_ATTRIBUTE_MODIFIED;
  return 0;
}

int
attribute_get_flags (attribute_t attr, int *pflags)
{
  if (attr == NULL)
    return EINVAL;
  if (attr->_get_flags)
    return attr->_get_flags (attr, pflags);
  if (pflags)
    *pflags = attr->flags;
  return 0;
}

int
attribute_set_flags (attribute_t attr, int flags)
{
  if (attr == NULL)
    return EINVAL;
  if (attr->_set_flags)
    attr->_set_flags (attr, flags);
  attr->flags |= flags;
  return 0;
}

int
attribute_set_get_flags (attribute_t attr, int (*_get_flags)
			 (attribute_t, int *), void *owner)
{
  if (attr == NULL)
    return EINVAL;
  if (attr->owner != owner)
    return EACCES;
  attr->_get_flags = _get_flags;
  return 0;
}

int
attribute_set_set_flags (attribute_t attr, int (*_set_flags)
			 (attribute_t, int), void *owner)
{
  if (attr == NULL)
    return EINVAL;
  if (attr->owner != owner)
    return EACCES;
  attr->_set_flags = _set_flags;
  return 0;
}

int
attribute_set_unset_flags (attribute_t attr, int (*_unset_flags)
			 (attribute_t, int), void *owner)
{
  if (attr == NULL)
    return EINVAL;
  if (attr->owner != owner)
    return EACCES;
  attr->_unset_flags = _unset_flags;
  return 0;
}

/* We add support for "USER" flag, it is a way for external objects
   Not being the owner to add custom flags.  */
int
attribute_set_userflag (attribute_t attr, int flag)
{
  if (attr == NULL)
    return EINVAL;
  attr->user_flags |= flag;
  return 0;
}

int
attribute_set_seen (attribute_t attr)
{
  int status = 0;
  if (attr == NULL)
    return EINVAL;
  if (attr->_set_flags)
    status = attr->_set_flags (attr, MU_ATTRIBUTE_SEEN);
  attr->flags |= MU_ATTRIBUTE_SEEN | MU_ATTRIBUTE_MODIFIED;
  return status;
}

int
attribute_set_answered (attribute_t attr)
{
  int status = 0;
  if (attr == NULL)
    return EINVAL;
  if (attr->_set_flags)
    status = attr->_set_flags (attr, MU_ATTRIBUTE_ANSWERED);
  attr->flags |= MU_ATTRIBUTE_ANSWERED | MU_ATTRIBUTE_MODIFIED;
  return status;
}

int
attribute_set_flagged (attribute_t attr)
{
  int status = 0;
  if (attr == NULL)
    return EINVAL;
  if (attr->_set_flags)
    status = attr->_set_flags (attr, MU_ATTRIBUTE_FLAGGED);
  attr->flags |= MU_ATTRIBUTE_FLAGGED | MU_ATTRIBUTE_MODIFIED;
  return status;
}

int
attribute_set_read (attribute_t attr)
{
  int status = 0;
  if (attr == NULL)
    return EINVAL;
  if (attr->_set_flags)
    status = attr->_set_flags (attr, MU_ATTRIBUTE_READ);
  attr->flags |= MU_ATTRIBUTE_READ | MU_ATTRIBUTE_MODIFIED;
  return status;
}

int
attribute_set_deleted (attribute_t attr)
{
  int status = 0;
  if (attr == NULL)
    return EINVAL;
  if (attr->_set_flags)
    status = attr->_set_flags (attr, MU_ATTRIBUTE_DELETED);
  attr->flags |= MU_ATTRIBUTE_DELETED | MU_ATTRIBUTE_MODIFIED;
  return 0;
}

int
attribute_set_draft (attribute_t attr)
{
  int status = 0;
  if (attr == NULL)
    return EINVAL;
  if (attr->_set_flags)
    status = attr->_set_flags (attr, MU_ATTRIBUTE_DRAFT);
  attr->flags |= MU_ATTRIBUTE_DRAFT | MU_ATTRIBUTE_MODIFIED;
  return status;
}

int
attribute_set_recent (attribute_t attr)
{
  int status = 0;
  if (attr == NULL)
    return EINVAL;
  if (attr->_unset_flags)
    {
      status |= attr->_unset_flags (attr, MU_ATTRIBUTE_READ);
      status |= attr->_unset_flags (attr, MU_ATTRIBUTE_SEEN);
    }
  if (attr == NULL)
    {
      attr->flags &= ~MU_ATTRIBUTE_READ;
      attr->flags &= ~MU_ATTRIBUTE_SEEN;
    }
  return status;
}

int
attribute_is_userflag (attribute_t attr, int flag)
{
  if (attr == NULL)
    return 0;
  return attr->user_flags & flag;
}

int
attribute_is_seen (attribute_t attr)
{
  if (attr == NULL)
    return 0;
  if (attr->_get_flags)
    attr->_get_flags (attr, &(attr->flags));
  return attr->flags & MU_ATTRIBUTE_SEEN;
}

int
attribute_is_answered (attribute_t attr)
{
  if (attr == NULL)
    return 0;
  if (attr->_get_flags)
    attr->_get_flags (attr, &(attr->flags));
  return attr->flags & MU_ATTRIBUTE_ANSWERED;
}

int
attribute_is_flagged (attribute_t attr)
{
  if (attr == NULL)
    return 0;
  if (attr->_get_flags)
    attr->_get_flags (attr, &(attr->flags));
  return attr->flags & MU_ATTRIBUTE_FLAGGED;
}

int
attribute_is_read (attribute_t attr)
{
  if (attr == NULL)
    return 0;
  if (attr->_get_flags)
    attr->_get_flags (attr, &(attr->flags));
  return attr->flags & MU_ATTRIBUTE_READ;
}

int
attribute_is_deleted (attribute_t attr)
{
  if (attr == NULL)
    return 0;
  if (attr->_get_flags)
    attr->_get_flags (attr, &(attr->flags));
  return attr->flags & MU_ATTRIBUTE_DELETED;
}

int
attribute_is_draft (attribute_t attr)
{
  if (attr == NULL)
    return 0;
  if (attr->_get_flags)
    attr->_get_flags (attr, &(attr->flags));
  return attr->flags & MU_ATTRIBUTE_DRAFT;
}

int
attribute_is_recent (attribute_t attr)
{
  if (attr == NULL)
    return 0;
  if (attr->_get_flags)
    attr->_get_flags (attr, &(attr->flags));
  /* something is recent when it is not read and not seen.  */
  return (attr->flags == 0
	  || ! ((attr->flags & MU_ATTRIBUTE_SEEN)
		&& (attr->flags & MU_ATTRIBUTE_READ)));
}

int
attribute_unset_userflag (attribute_t attr, int flag)
{
  if (attr == NULL)
    return 0;
  attr->user_flags &= ~flag;
  return 0;
}

int
attribute_unset_seen (attribute_t attr)
{
  int status = 0;
  if (attr == NULL)
    return 0;
  if (attr->_unset_flags)
    status = attr->_unset_flags (attr, MU_ATTRIBUTE_SEEN);
  attr->flags &= ~MU_ATTRIBUTE_SEEN;
  return status;
}

int
attribute_unset_answered (attribute_t attr)
{
  int status = 0;
  if (attr == NULL)
    return 0;
  if (attr->_unset_flags)
    status = attr->_unset_flags (attr, MU_ATTRIBUTE_ANSWERED);
  attr->flags &= ~MU_ATTRIBUTE_ANSWERED;
  return status;
}

int
attribute_unset_flagged (attribute_t attr)
{
  int status = 0;
  if (attr == NULL)
    return 0;
  if (attr->_unset_flags)
    status = attr->_unset_flags (attr, MU_ATTRIBUTE_FLAGGED);
  attr->flags &= ~MU_ATTRIBUTE_FLAGGED;
  return status;
}

int
attribute_unset_read (attribute_t attr)
{
  int status = 0;
  if (attr == NULL)
    return 0;
  if (attr->_unset_flags)
    status = attr->_unset_flags (attr, MU_ATTRIBUTE_READ);
  attr->flags &= ~MU_ATTRIBUTE_READ;
  return status;
}

int
attribute_unset_deleted (attribute_t attr)
{
  int status = 0;
  if (attr == NULL)
    return 0;
  if (attr->_unset_flags)
    status = attr->_unset_flags (attr, MU_ATTRIBUTE_DELETED);
  attr->flags &= ~MU_ATTRIBUTE_DELETED;
  return status;
}

int
attribute_unset_draft (attribute_t attr)
{
  int status = 0;
  if (attr == NULL)
    return 0;
  if (attr->_unset_flags)
    status = attr->_unset_flags (attr, MU_ATTRIBUTE_DRAFT);
  attr->flags &= ~MU_ATTRIBUTE_DRAFT;
  return status;
}

int
attribute_unset_recent (attribute_t attr)
{
  int status = 0;
  if (attr == NULL)
    return 0;
  if (attr->_unset_flags)
    status = attr->_unset_flags (attr, MU_ATTRIBUTE_SEEN);
  attr->flags |= MU_ATTRIBUTE_SEEN;
  return status;
}

int
attribute_is_equal (attribute_t attr, attribute_t attr2)
{
  int status = 0;
  if (attr == NULL || attr2 == NULL)
    return 0;
  if (attr->_get_flags)
    status = attr->_get_flags (attr, &(attr->flags));
  return attr->flags == attr2->flags;
}

/*   Miscellaneous.  */
int
attribute_copy (attribute_t dest, attribute_t src)
{
  if (dest == NULL || src == NULL)
    return EINVAL;
  memcpy (dest, src, sizeof (*dest));
  return 0;
}

int
string_to_flags (const char *buffer, int *pflags)
{
  const char *sep;

  if (pflags == NULL)
    return EINVAL;

  /* Set the attribute */
  if (strncasecmp (buffer, "Status:", 7) == 0)
    {
      sep = strchr(buffer, ':'); /* pass the ':' */
      sep++;
    }
  else
    sep = buffer;

  while (*sep)
    {
      if (strchr (sep, 'R') != NULL || strchr (sep, 'r') != NULL)
	*pflags |= MU_ATTRIBUTE_READ;
      if (strchr (sep, 'O') != NULL || strchr (sep, 'o') != NULL)
	*pflags |= MU_ATTRIBUTE_SEEN;
      if (strchr (sep, 'A') != NULL || strchr (sep, 'a') != NULL)
	*pflags |= MU_ATTRIBUTE_ANSWERED;
      if (strchr (sep, 'F') != NULL || strchr (sep, 'f') != NULL)
	*pflags |= MU_ATTRIBUTE_FLAGGED;
      sep++;
    }
  return 0;
}

int
attribute_to_string (attribute_t attr, char *buffer, size_t len, size_t *pn)
{
  int flags = 0;;
  attribute_get_flags (attr, &flags);
  return flags_to_string (flags, buffer, len, pn);
}

static int
flags_to_string (int flags, char *buffer, size_t len, size_t *pn)
{
  char status[32];
  char a[8];
  size_t i;

  *status = *a = '\0';

  if (flags & MU_ATTRIBUTE_SEEN)
    strcat (a, "R");
  if (flags & MU_ATTRIBUTE_ANSWERED)
    strcat (a, "A");
  if (flags & MU_ATTRIBUTE_FLAGGED)
    strcat (a, "F");
  if (flags & MU_ATTRIBUTE_READ)
    strcat (a, "O");
  if (flags & MU_ATTRIBUTE_DELETED)
    strcat (a, "d");

  if (*a != '\0')
    {
      strcpy (status, "Status: ");
      strcat (status, a);
      strcat (status, "\n");
    }

  i = strlen (status);

  if (buffer && len != 0)
    {
      strncpy (buffer, status, len - 1);
      buffer[len - 1] = '\0';
      i = strlen (buffer);
    }
  if (pn)
    *pn = i;
  return 0;
}
