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

#include <locker.h>
#include <errno.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>

/*
 * Waiting for Brian E. to implement this.
 */

struct _locker
{
  char *filename;
  size_t filename_len;
  int lock;
};

int
locker_init (locker_t *plocker, char *filename, size_t len)
{
  locker_t l;
  if (plocker == NULL)
    return EINVAL;
  if (filename == NULL || len == 0)
    return EINVAL;
  l = malloc (sizeof(*l));
  if (l == NULL)
    return ENOMEM;
  l->filename = calloc (len + 1, sizeof(char));
  if (l->filename == NULL)
    {
      free (l);
      return ENOMEM;
    }
  memcpy (l->filename, filename, len);
  l->lock = 0;
  l->filename_len = len;
  *plocker = l;
  return 0;
}

void
locker_destroy (locker_t *plocker)
{
  if (plocker && *plocker)
    {
      free ((*plocker)->filename);
      free (*plocker);
      *plocker = NULL;
    }
}

int
locker_lock (locker_t locker, int flags)
{
  (void)flags;
  if (locker == NULL)
    return EINVAL;
  locker->lock++;
  return 0;
}

int
locker_touchlock (locker_t locker)
{
  if (locker == NULL)
    return EINVAL;
  return 0;
}

int
locker_unlock (locker_t locker)
{
  if (locker == NULL)
    return EINVAL;
  locker->lock--;
  return 0;
}
