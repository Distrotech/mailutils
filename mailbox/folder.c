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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <errno.h>
#include <stdlib.h>

#include <mailutils/registrar.h>
#include <mailutils/iterator.h>

#include <misc.h>
#include <folder0.h>

int
folder_create (folder_t *pfolder, const char *name)
{
  int status = EINVAL;
  record_t record = NULL;
  folder_entry_t entry = NULL;
  iterator_t iterator;
  list_t list;
  int found = 0;

  if (pfolder == NULL)
    return EINVAL;

  /* Look in the registrar, for a match  */
  registrar_get_list (&list);
  status = iterator_create (&iterator, list);
  if (status != 0)
    return status;
  for (iterator_first (iterator); !iterator_is_done (iterator);
       iterator_next (iterator))
    {
      iterator_current (iterator, (void **)&record);
      if (record_is_scheme (record, name))
        {
          record_get_folder (record, &entry);
          if (entry)
	    {
	      found = 1;
	      break;
	    }
        }
    }
  iterator_destroy (&iterator);

  if (found)
    {
      url_t url = NULL;
      folder_t folder = NULL;

      /* Allocate memory for mbox.  */
      folder = calloc (1, sizeof (*folder));
      if (folder == NULL)
        return ENOMEM;

      /* Initialize the internal lock, now so the concrete mailbox
         could use it. */
      status = RWLOCK_INIT (&(folder->rwlock), NULL);
      if (status != 0)
        {
          folder_destroy (&folder);
          return status;
        }

      /* Parse the url, it may be a bad one and we should bailout if this
         failed.  */
      if ((status = url_create (&url, name)) != 0
          || (status = entry->_url_init (url)) != 0)
        {
          folder_destroy (&folder);
          return status;
        }
      folder->url = url;

      /* Create the concrete folder type.  */
      status = entry->_folder_init (folder);
      if (status != 0)
        {
          folder_destroy (&folder);
        }
      else
        *pfolder = folder;
    }
  else
    status = ENOENT;

  return status;
}

void
folder_destroy (folder_t *pfolder)
{
  if (pfolder && *pfolder)
    {
      folder_t folder = *pfolder;
      int destroy_lock = 0;
#ifdef WITH_PTHREAD
      pthread_rwlock_t rwlock = folder->rwlock;
#endif
      RWLOCK_WRLOCK (&(rwlock));
      if (folder_decremente (folder) == 0)
	{
	  RWLOCK_UNLOCK (&(rwlock));
	  destroy_lock = 1;
	  /* Notify the observers.  */
	  if (folder->observable)
	    {
	      observable_notify (folder->observable, MU_EVT_FOLDER_DESTROY);
	      observable_destroy (&(folder->observable), folder);
	    }
	  if (folder->_destroy)
	    folder->_destroy (folder);
	  RWLOCK_WRLOCK (&(rwlock));
	  if (folder->ticket)
	    ticket_destroy (&(folder->ticket), folder);
	  if (folder->authority)
	    authority_destroy (&(folder->authority), folder);
	  if (folder->stream)
	    stream_destroy (&(folder->stream), folder);
	  if (folder->url)
	    url_destroy (&(folder->url));
	  free (folder);
	}
      RWLOCK_UNLOCK (&(rwlock));
      if (destroy_lock)
	RWLOCK_DESTROY (&(rwlock));
      *pfolder = NULL;
    }
}

int
folder_open (folder_t folder, int flags)
{
  if (folder == NULL || folder->_open == NULL)
    return ENOSYS;
  return folder->_open (folder, flags);
}

int
folder_close (folder_t folder)
{
  if (folder == NULL || folder->_close == NULL)
    return ENOSYS;
  return folder->_close (folder);
}

int
folder_set_stream (folder_t folder, stream_t stream)
{
  if (folder == NULL)
    return EINVAL;
  if (folder->stream)
    stream_destroy (&(folder->stream), folder);
  folder->stream = stream;
  return 0;
}

int
folder_get_stream (folder_t folder, stream_t *pstream)
{
  if (folder == NULL || pstream == NULL)
    return EINVAL;
  *pstream = folder->stream;
  return 0;
}

int
folder_set_authority (folder_t folder, authority_t authority)
{
  if (folder == NULL)
    return EINVAL;
  if (folder->authority)
    authority_destroy (&(folder->authority), folder);
  folder->authority = authority;
  return 0;
}

int
folder_get_authority (folder_t folder, authority_t *pauthority)
{
  if (folder == NULL || pauthority == NULL)
    return EINVAL;
  *pauthority = folder->authority;
  return 0;
}

int
folder_set_ticket (folder_t folder, ticket_t ticket)
{
  if (folder == NULL)
    return EINVAL;
  if (folder->ticket)
    ticket_destroy (&(folder->ticket), folder);
  folder->ticket = ticket;
  return 0;
}

int
folder_get_ticket (folder_t folder, ticket_t *pticket)
{
  if (folder == NULL || pticket == NULL)
    return EINVAL;
  *pticket = folder->ticket;
  return 0;
}
int
folder_get_observable (folder_t folder, observable_t *pobservable)
{
  if (folder == NULL  || pobservable == NULL)
    return EINVAL;

  if (folder->observable == NULL)
    {
      int status = observable_create (&(folder->observable), folder);
      if (status != 0)
        return status;
    }
  *pobservable = folder->observable;
  return 0;
}

int
folder_set_debug (folder_t folder, debug_t debug)
{
  if (folder == NULL)
    return EINVAL;
  if (folder->debug)
    debug_destroy (&(folder->debug), folder);
  folder->debug = debug;
  return 0;
}

int
folder_get_debug (folder_t folder, debug_t *pdebug)
{
  if (folder == NULL || pdebug == NULL)
    return EINVAL;
  if (folder->debug == NULL)
    {
      int status = debug_create (&(folder->debug), folder);
      if (status != 0)
        return status;
    }
  *pdebug = folder->debug;
  return 0;
}


int
folder_list (folder_t folder, list_t *plist)
{
  if (folder == NULL || folder->_list == NULL)
    return ENOSYS;
  return folder->_list (folder, plist);
}

int
folder_delete_mailbox (folder_t folder, const char *name)
{
  if (folder == NULL || folder->_delete_mailbox == NULL)
    return ENOSYS;
  return folder->_delete_mailbox (folder, name);
}

int
folder_decremente (folder_t folder)
{
  if (folder && folder->_decremente)
    return folder->_decremente (folder);
  return (folder->ref--);
}
