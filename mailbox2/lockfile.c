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

#include <stdlib.h>
#include <mailutils/error.h>
#include <mailutils/sys/lockfile.h>

int
lockfile_ref (lockfile_t lockfile)
{
  if (lockfile == NULL || lockfile->vtable == NULL
      || lockfile->vtable->ref == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return lockfile->vtable->ref (lockfile);
}

void
lockfile_destroy (lockfile_t *plockfile)
{
  if (plockfile && *plockfile)
    {
      lockfile_t lockfile = *plockfile;
      if (lockfile->vtable && lockfile->vtable->destroy)
	lockfile->vtable->destroy (plockfile);
      *plockfile = NULL;
    }
}

int
lockfile_lock (lockfile_t lockfile)
{
  if (lockfile == NULL || lockfile->vtable == NULL
      || lockfile->vtable->lock == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return lockfile->vtable->lock (lockfile);
}

int
lockfile_touchlock (lockfile_t lockfile)
{
  if (lockfile == NULL || lockfile->vtable == NULL
      || lockfile->vtable->lock == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return lockfile->vtable->lock (lockfile);
}

int
lockfile_unlock (lockfile_t lockfile)
{
  if (lockfile == NULL || lockfile->vtable == NULL
      || lockfile->vtable->lock == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return lockfile->vtable->lock (lockfile);
}
