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

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <mailutils/registrar.h>
#include <mailutils/iterator.h>
#include <mailutils/list.h>
#include <mailutils/monitor.h>

#include <folder0.h>

/* Internal folder list.  */
static list_t known_folder_list;
static int is_known_folder __P ((url_t, folder_t *));

/* Static folder lock.  */
static struct _monitor folder_lock = MU_MONITOR_INITIALIZER;

/* A folder could be remote (IMAP), or local(a spool directory) like $HOME/Mail
   etc ..  We maintain a known list of folder to not generate multiple folder
   of the same URL.  Meaning when folder_create () is call we'll check if we
   already have a folder for that URL and return the same, if not we create a
   new one.  The downside, the scheme to detect the same URL is very weak, and
   they maybe cases where you want a different folder for the same URL, there
   is not easy way to do this.  */
int
folder_create (folder_t *pfolder, const char *name)
{
  int status = EINVAL;
  record_t record = NULL;
  int (*f_init) __P ((folder_t)) = NULL;
  int (*u_init) __P ((url_t)) = NULL;
  iterator_t iterator;
  list_t list;
  int found = 0;

  if (pfolder == NULL)
    return EINVAL;

  /* Look in the registrar list(iterator), for a possible concrete mailbox
     implementatio that could match the URL.  */
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
          record_get_folder (record, &f_init);
          record_get_url (record, &u_init);
          if (f_init && u_init)
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

      /* Parse the url, it may be a bad one and we should bailout if this
	 failed.  */
      if ((status = url_create (&url, name) != 0)
          || (status = u_init (url)) != 0)
	return status;

      monitor_wrlock (&folder_lock);

      /* Check if we already have the same URL folder.  */
      if (is_known_folder (url, &folder))
	{
	  folder->ref++;
	  *pfolder = folder;
	  url_destroy (&url);
	  monitor_unlock (&folder_lock);
	  return  0;
	}
      else
	monitor_unlock (&folder_lock);

      /* Create a new folder.  */

      /* Allocate memory for folder.  */
      folder = calloc (1, sizeof (*folder));
      if (folder != NULL)
	{
	  folder->url = url;
	  /* Initialize the internal foilder lock, now so the concrete folder
	     could use it.  */
	  status = monitor_create (&(folder->monitor), 0, folder);
	  if (status == 0)
	    {
	      /* Create the concrete folder type.  */
	      status = f_init (folder);
	      if (status == 0)
		{
		  *pfolder = folder;
		  folder->ref++;
		  /* Put on the internal list of known folders.  */
		  if (known_folder_list == NULL)
		    list_create (&known_folder_list);
		  list_append (known_folder_list, folder);
		}
	    }
	  /* Something went wrong, destroy the object. */
	  if (status != 0)
	    {
	      if (folder->monitor)
		monitor_destroy (&(folder->monitor), folder);
	      if (folder->url)
		url_destroy (&(folder->url));
	      free (folder);
	    }
	}
    }
  else
    status = ENOENT;

  return status;
}

/* The folder is destroy if it is the last reference.  */
void
folder_destroy (folder_t *pfolder)
{
  if (pfolder && *pfolder)
    {
      folder_t folder = *pfolder;
      int destroy_lock = 0;
      monitor_t monitor = folder->monitor;

      monitor_wrlock (monitor);

      /* Check if this the last reference for this folder.  If yes removed
         it from the list.  */
      monitor_wrlock (&folder_lock);
      folder->ref--;
      /* Remove the folder from the list of known folder.  */
      if (folder->ref <= 0)
	list_remove (known_folder_list, folder);
      /* If the list is empty we can safely remove it.  */
      if (list_is_empty)
	{
	  list_destroy (&known_folder_list);
	  known_folder_list = NULL;
	}
      monitor_unlock (&folder_lock);

      if (folder->ref <= 0)
	{
	  monitor_unlock (monitor);
	  destroy_lock = 1;
	  /* Notify the observers.  */
	  if (folder->observable)
	    {
	      observable_notify (folder->observable, MU_EVT_FOLDER_DESTROY);
	      observable_destroy (&(folder->observable), folder);
	    }
	  if (folder->_destroy)
	    folder->_destroy (folder);
	  monitor_wrlock (monitor);
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
      monitor_unlock (monitor);
      if (destroy_lock)
	monitor_destroy (&monitor, folder);
      *pfolder = NULL;
    }
}

/* Cover functions.  */
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
  if (folder->authority)
    {
      authority_set_ticket (folder->authority, ticket);
    }
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
folder_set_debug (folder_t folder, mu_debug_t debug)
{
  if (folder == NULL)
    return EINVAL;
  if (folder->debug)
    mu_debug_destroy (&(folder->debug), folder);
  folder->debug = debug;
  return 0;
}

int
folder_get_debug (folder_t folder, mu_debug_t *pdebug)
{
  if (folder == NULL || pdebug == NULL)
    return EINVAL;
  if (folder->debug == NULL)
    {
      int status = mu_debug_create (&(folder->debug), folder);
      if (status != 0)
        return status;
    }
  *pdebug = folder->debug;
  return 0;
}

int
folder_list (folder_t folder, const char *dirname, const char *basename,
	     struct folder_list *pflist)
{
  if (folder == NULL || folder->_list == NULL)
    return EINVAL;
  return folder->_list (folder, dirname, basename, pflist);
}

int
folder_lsub (folder_t folder, const char *dirname, const char *basename,
	     struct folder_list *pflist)
{
  if (folder == NULL || folder->_lsub == NULL)
    return ENOSYS;
  return folder->_lsub (folder, dirname, basename, pflist);
}

int
folder_subscribe (folder_t folder, const char *name)
{
  if (folder == NULL || folder->_subscribe == NULL)
    return EINVAL;
  return folder->_subscribe (folder, name);
}

int
folder_unsubscribe (folder_t folder, const char *name)
{
  if (folder == NULL || folder->_unsubscribe == NULL)
    return EINVAL;
  return folder->_unsubscribe (folder, name);
}

int
folder_list_destroy (struct folder_list *pflist)
{
  size_t i;
  if (pflist == NULL)
    return 0;
  for (i = 0 ; i < pflist->num; i++)
    {
      if (pflist->element[i])
	{
	  if (pflist->element[i]->name)
	    free (pflist->element[i]->name);
	  free (pflist->element[i]);
	}
    }
  if (i > 0)
    free (pflist->element);
  pflist->element = NULL;
  pflist->num = 0;
  return 0;
}

int
folder_delete (folder_t folder, const char *name)
{
  if (folder == NULL || folder->_delete == NULL)
    return ENOSYS;
  return folder->_delete (folder, name);
}

int
folder_rename (folder_t folder, const char *oldname, const char *newname)
{
  if (folder == NULL || folder->_rename == NULL)
    return ENOSYS;
  return folder->_rename (folder, oldname, newname);
}

int
folder_get_url (folder_t folder, url_t *purl)
{
  if (folder == NULL || purl == NULL)
    return EINVAL;
  *purl = folder->url;
  return 0;
}

static int is_known_folder (url_t url, folder_t *pfolder)
{
  int ret = 0;
  folder_t folder = NULL;
  iterator_t iterator;

  if (url == NULL || pfolder == NULL)
    return ret;

  if (iterator_create (&iterator, known_folder_list) != 0)
    return ret;

  for (iterator_first (iterator); !iterator_is_done (iterator);
       iterator_next (iterator))
    {
      iterator_current (iterator, (void **)&folder);
      /* Check if the same URL type.  */
      if (folder && folder->url
	  && url_is_same_scheme (url, folder->url)
	  && url_is_same_user (url, folder->url)
	  && url_is_same_host (url, folder->url)
	  /*&& url_is_same_path (url, folder->url) */
	  && url_is_same_port (url, folder->url))
	{
	  ret = 1;
	  break;
	}
    }
  if (ret)
    *pfolder = folder;
  iterator_destroy (&iterator);
  return ret;
}
