/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA  */

#include <stdlib.h>
#include <mailutils/error.h>
#include <mailutils/sys/folder.h>

int
folder_ref (folder_t folder)
{
  if (folder == NULL || folder->vtable == NULL
      || folder->vtable->ref == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return folder->vtable->ref (folder);
}

void
folder_destroy (folder_t *pfolder)
{
  if (pfolder && *pfolder)
    {
      folder_t folder = *pfolder;
      if (folder->vtable && folder->vtable->destroy)
	folder->vtable->destroy (pfolder);
      *pfolder = NULL;
    }
}

int
folder_open (folder_t folder, int flag)
{
  if (folder == NULL || folder->vtable == NULL
      || folder->vtable->open == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return folder->vtable->open (folder, flag);
}

int
folder_close (folder_t folder)
{
  if (folder == NULL || folder->vtable == NULL
      || folder->vtable->close == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return folder->vtable->close (folder);
}

int
folder_delete (folder_t folder, const char *name)
{
  if (folder == NULL || folder->vtable == NULL
      || folder->vtable->delete == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return folder->vtable->delete (folder, name);
}

int
folder_rename (folder_t folder, const char *oldname, const char *newname)
{
  if (folder == NULL || folder->vtable == NULL
      || folder->vtable->rename == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return folder->vtable->rename (folder, oldname, newname);
}

int
folder_subscribe (folder_t folder, const char *name)
{
  if (folder == NULL || folder->vtable == NULL
      || folder->vtable->subscribe == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return folder->vtable->subscribe (folder, name);
}

int
folder_unsubscribe (folder_t folder, const char *name)
{
  if (folder == NULL || folder->vtable == NULL
      || folder->vtable->unsubscribe == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return folder->vtable->unsubscribe (folder, name);
}

int
folder_list (folder_t folder, const char *dir, const char *name,
	      iterator_t *iterator)
{
  if (folder == NULL || folder->vtable == NULL
      || folder->vtable->list == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return folder->vtable->list (folder, dir, name, iterator);
}

int
folder_lsub (folder_t folder, const char *dir, const char *name,
	     iterator_t *iterator)
{
  if (folder == NULL || folder->vtable == NULL
      || folder->vtable->lsub == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return folder->vtable->lsub (folder, dir, name, iterator);
}

/* Stream settings.  */
int
folder_get_stream (folder_t folder, stream_t *stream)
{
  if (folder == NULL || folder->vtable == NULL
      || folder->vtable->get_stream == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return folder->vtable->get_stream (folder, stream);
}

int
folder_set_stream (folder_t folder, stream_t stream)
{
  if (folder == NULL || folder->vtable == NULL
      || folder->vtable->set_stream == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return folder->vtable->set_stream (folder, stream);
}


/* Notifications.  */
int
folder_get_observable (folder_t folder, observable_t *observable)
{
  if (folder == NULL || folder->vtable == NULL
      || folder->vtable->get_observable == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return folder->vtable->get_observable (folder, observable);
}

int
folder_get_debug (folder_t folder, mu_debug_t *debug)
{
  if (folder == NULL || folder->vtable == NULL
      || folder->vtable->get_debug == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return folder->vtable->get_debug (folder, debug);
}

int
folder_set_debug (folder_t folder, mu_debug_t debug)
{
  if (folder == NULL || folder->vtable == NULL
      || folder->vtable->set_debug == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return folder->vtable->set_debug (folder, debug);
}


/* Authentication.  */
int
folder_get_authority (folder_t folder, authority_t *authority)
{
  if (folder == NULL || folder->vtable == NULL
      || folder->vtable->get_authority == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return folder->vtable->get_authority (folder, authority);
}

int
folder_set_authority (folder_t folder, authority_t authority)
{
  if (folder == NULL || folder->vtable == NULL
      || folder->vtable->set_authority == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return folder->vtable->set_authority (folder, authority);
}


/* URL.  */
int
folder_get_url (folder_t folder, url_t *url)
{
  if (folder == NULL || folder->vtable == NULL
      || folder->vtable->get_url == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return folder->vtable->get_url (folder, url);
}
