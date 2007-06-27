/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2004, 2005, 
   2007 Free Software Foundation, Inc.

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

#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif

#include <mailutils/errno.h>
#include <attribute0.h>

static int flags_to_string (int, char *, size_t, size_t *);

int
mu_attribute_create (mu_attribute_t *pattr, void *owner)
{
  mu_attribute_t attr;
  if (pattr == NULL)
    return MU_ERR_OUT_PTR_NULL;
  attr = calloc (1, sizeof(*attr));
  if (attr == NULL)
    return ENOMEM;
  attr->owner = owner;
  *pattr = attr;
  return 0;
}

void
mu_attribute_destroy (mu_attribute_t *pattr, void *owner)
{
  if (pattr && *pattr)
    {
      mu_attribute_t attr = *pattr;
      if (attr->owner == owner)
	free (*pattr);
      /* Loose the link */
      *pattr = NULL;
    }
}

void *
mu_attribute_get_owner (mu_attribute_t attr)
{
  return (attr) ? attr->owner : NULL;
}

int
mu_attribute_is_modified (mu_attribute_t attr)
{
  return (attr) ? attr->flags & MU_ATTRIBUTE_MODIFIED : 0;
}

int
mu_attribute_clear_modified (mu_attribute_t attr)
{
  if (attr)
    attr->flags &= ~MU_ATTRIBUTE_MODIFIED;
  return 0;
}

int
mu_attribute_set_modified (mu_attribute_t attr)
{
  if (attr)
    attr->flags |= MU_ATTRIBUTE_MODIFIED;
  return 0;
}

int
mu_attribute_get_flags (mu_attribute_t attr, int *pflags)
{
  if (attr == NULL)
    return EINVAL;
  if (pflags == NULL)
    return MU_ERR_OUT_PTR_NULL;
  if (attr->_get_flags)
    return attr->_get_flags (attr, pflags);
  *pflags = attr->flags;
  return 0;
}

int
mu_attribute_set_flags (mu_attribute_t attr, int flags)
{
  int status = 0;
  int oflags = 0;
  
  if (attr == NULL)
    return EINVAL;

  /* If the required bits are already set, do not modify anything */
  mu_attribute_get_flags (attr, &oflags);
  if ((oflags & flags) == flags)
    return 0;
  
  if (attr->_set_flags)
    status = attr->_set_flags (attr, flags);
  else
    attr->flags |= flags;
  if (status == 0)
    mu_attribute_set_modified (attr);
  return 0;
}

int
mu_attribute_unset_flags (mu_attribute_t attr, int flags)
{
  int status = 0;
  int oflags = 0;

  if (attr == NULL)
    return EINVAL;

  /* If the required bits are already cleared, do not modify anything */
  mu_attribute_get_flags (attr, &oflags);
  if ((oflags & flags) == 0)
    return 0;

  if (attr->_unset_flags)
    status = attr->_unset_flags (attr, flags);
  else
    attr->flags &= ~flags;
  if (status == 0)
    mu_attribute_set_modified (attr);
  return 0;
}

int
mu_attribute_set_get_flags (mu_attribute_t attr, int (*_get_flags)
			 (mu_attribute_t, int *), void *owner)
{
  if (attr == NULL)
    return EINVAL;
  if (attr->owner != owner)
    return EACCES;
  attr->_get_flags = _get_flags;
  return 0;
}

int
mu_attribute_set_set_flags (mu_attribute_t attr, int (*_set_flags)
			 (mu_attribute_t, int), void *owner)
{
  if (attr == NULL)
    return EINVAL;
  if (attr->owner != owner)
    return EACCES;
  attr->_set_flags = _set_flags;
  return 0;
}

int
mu_attribute_set_unset_flags (mu_attribute_t attr, int (*_unset_flags)
			 (mu_attribute_t, int), void *owner)
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
mu_attribute_set_userflag (mu_attribute_t attr, int flag)
{
  if (attr == NULL)
    return EINVAL;
  attr->user_flags |= flag;
  return 0;
}

int
mu_attribute_set_seen (mu_attribute_t attr)
{
  return mu_attribute_set_flags (attr, MU_ATTRIBUTE_SEEN);
}

int
mu_attribute_set_answered (mu_attribute_t attr)
{
  return mu_attribute_set_flags (attr, MU_ATTRIBUTE_ANSWERED);
}

int
mu_attribute_set_flagged (mu_attribute_t attr)
{
  return mu_attribute_set_flags (attr, MU_ATTRIBUTE_FLAGGED);
}

int
mu_attribute_set_read (mu_attribute_t attr)
{
  return mu_attribute_set_flags (attr, MU_ATTRIBUTE_READ);
}

int
mu_attribute_set_deleted (mu_attribute_t attr)
{
  return mu_attribute_set_flags (attr, MU_ATTRIBUTE_DELETED);
}

int
mu_attribute_set_draft (mu_attribute_t attr)
{
  return mu_attribute_set_flags (attr, MU_ATTRIBUTE_DRAFT);
}

int
mu_attribute_set_recent (mu_attribute_t attr)
{
  int status = mu_attribute_unset_flags (attr, MU_ATTRIBUTE_READ);
  if (status == 0)
    status = mu_attribute_unset_flags (attr, MU_ATTRIBUTE_SEEN);
  return status;
}

int
mu_attribute_is_userflag (mu_attribute_t attr, int flag)
{
  if (attr == NULL)
    return 0;
  return attr->user_flags & flag;
}

int
mu_attribute_is_seen (mu_attribute_t attr)
{
  int flags = 0;
  if (mu_attribute_get_flags (attr, &flags) == 0)
    return flags & MU_ATTRIBUTE_SEEN;
  return 0;
}

int
mu_attribute_is_answered (mu_attribute_t attr)
{
  int flags = 0;
  if (mu_attribute_get_flags (attr, &flags) == 0)
    return flags & MU_ATTRIBUTE_ANSWERED;
  return 0;
}

int
mu_attribute_is_flagged (mu_attribute_t attr)
{
  int flags = 0;
  if (mu_attribute_get_flags (attr, &flags) == 0)
    return flags & MU_ATTRIBUTE_FLAGGED;
  return 0;
}

int
mu_attribute_is_read (mu_attribute_t attr)
{
  int flags = 0;
  if (mu_attribute_get_flags (attr, &flags) == 0)
    return flags & MU_ATTRIBUTE_READ;
  return 0;
}

int
mu_attribute_is_deleted (mu_attribute_t attr)
{
  int flags = 0;
  if (mu_attribute_get_flags (attr, &flags) == 0)
    return flags & MU_ATTRIBUTE_DELETED;
  return 0;
}

int
mu_attribute_is_draft (mu_attribute_t attr)
{
  int flags = 0;
  if (mu_attribute_get_flags (attr, &flags) == 0)
    return flags & MU_ATTRIBUTE_DRAFT;
  return 0;
}

int
mu_attribute_is_recent (mu_attribute_t attr)
{
  int flags = 0;
  if (mu_attribute_get_flags (attr, &flags) == 0)
    return MU_ATTRIBUTE_IS_UNSEEN(flags);
  return 0;
}

int
mu_attribute_unset_userflag (mu_attribute_t attr, int flag)
{
  if (attr == NULL)
    return 0;
  attr->user_flags &= ~flag;
  return 0;
}

int
mu_attribute_unset_seen (mu_attribute_t attr)
{
  return mu_attribute_unset_flags (attr, MU_ATTRIBUTE_SEEN);
}

int
mu_attribute_unset_answered (mu_attribute_t attr)
{
  return mu_attribute_unset_flags (attr, MU_ATTRIBUTE_ANSWERED);
}

int
mu_attribute_unset_flagged (mu_attribute_t attr)
{
  return mu_attribute_unset_flags (attr, MU_ATTRIBUTE_FLAGGED);
}

int
mu_attribute_unset_read (mu_attribute_t attr)
{
  return mu_attribute_unset_flags (attr, MU_ATTRIBUTE_READ);
}

int
mu_attribute_unset_deleted (mu_attribute_t attr)
{
  return mu_attribute_unset_flags (attr, MU_ATTRIBUTE_DELETED);
}

int
mu_attribute_unset_draft (mu_attribute_t attr)
{
  return mu_attribute_unset_flags (attr, MU_ATTRIBUTE_DRAFT);
}

int
mu_attribute_unset_recent (mu_attribute_t attr)
{
  return mu_attribute_unset_flags (attr, MU_ATTRIBUTE_SEEN);
}

int
mu_attribute_is_equal (mu_attribute_t attr, mu_attribute_t attr2)
{
  int flags2 = 0, flags = 0;
  mu_attribute_get_flags (attr, &flags);
  mu_attribute_get_flags (attr2, &flags2);
  return flags == flags;
}

/*   Miscellaneous.  */
int
mu_attribute_copy (mu_attribute_t dest, mu_attribute_t src)
{
  if (dest == NULL || src == NULL)
    return EINVAL;
  /* Can not be a deep copy.  */
  /* memcpy (dest, src, sizeof (*dest)); */
  dest->flags = src->flags;
  return 0;
}

int
mu_string_to_flags (const char *buffer, int *pflags)
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
mu_attribute_to_string (mu_attribute_t attr, char *buffer, size_t len, size_t *pn)
{
  int flags = 0;;
  mu_attribute_get_flags (attr, &flags);
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
    strcat (a, "O");
  if (flags & MU_ATTRIBUTE_ANSWERED)
    strcat (a, "A");
  if (flags & MU_ATTRIBUTE_FLAGGED)
    strcat (a, "F");
  if (flags & MU_ATTRIBUTE_READ)
    strcat (a, "R");
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

