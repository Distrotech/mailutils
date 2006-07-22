/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2004, 2005, 2006
   Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

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

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <mailutils/auth.h>
#include <mailutils/debug.h>
#include <mailutils/iterator.h>
#include <mailutils/list.h>
#include <mailutils/monitor.h>
#include <mailutils/observer.h>
#include <mailutils/registrar.h>
#include <mailutils/stream.h>
#include <mailutils/url.h>
#include <mailutils/errno.h>

#include <folder0.h>

/* Internal folder list.  */
static mu_list_t known_folder_list;
static int is_known_folder (mu_url_t, mu_folder_t *);

/* Static folder lock.  */
static struct mu_monitor folder_lock = MU_MONITOR_INITIALIZER;

/* A folder could be remote (IMAP), or local(a spool directory) like $HOME/Mail
   etc ..  We maintain a known list of folder to not generate multiple folder
   of the same URL.  Meaning when mu_folder_create () is call we'll check if we
   already have a folder for that URL and return the same, if not we create a
   new one.  The downside, the scheme to detect the same URL is very weak, and
   they maybe cases where you want a different folder for the same URL, there
   is not easy way to do this.  */
int
mu_folder_create_from_record (mu_folder_t *pfolder, const char *name,
			      mu_record_t record)
{
  if (!pfolder)
    return MU_ERR_OUT_PTR_NULL;

  if (record || 
      /* Look in the registrar list(iterator), for a possible concrete mailbox
	 implementation that could match the URL.  */
      mu_registrar_lookup (name, MU_FOLDER_ATTRIBUTE_DIRECTORY, &record, NULL)
      == 0)
    {
      int (*f_init) (mu_folder_t) = NULL;
      int (*u_init) (mu_url_t) = NULL;
      
      mu_record_get_folder (record, &f_init);
      mu_record_get_url (record, &u_init);
      if (f_init && u_init)
        {
	  int status;
	  mu_url_t url;
	  mu_folder_t folder;

	  /* Parse the url, it may be a bad one and we should bailout if this
	     failed.  */
	  if ((status = mu_url_create (&url, name) != 0)
	      || (status = u_init (url)) != 0)
	    return status;

	  mu_monitor_wrlock (&folder_lock);

	  /* Check if we already have the same URL folder.  */
	  if (is_known_folder (url, &folder))
	    {
	      folder->ref++;
	      *pfolder = folder;
	      mu_url_destroy (&url);
	      mu_monitor_unlock (&folder_lock);
	      return  0;
	    }
	  else
	    mu_monitor_unlock (&folder_lock);
	  
	  /* Create a new folder.  */

	  /* Allocate memory for the folder.  */
	  folder = calloc (1, sizeof (*folder));
	  if (folder != NULL)
	    {
	      folder->url = url;
	      /* Initialize the internal foilder lock, now so the
		 concrete folder could use it.  */
	      status = mu_monitor_create (&folder->monitor, 0, folder);
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
			mu_list_create (&known_folder_list);
		      mu_list_append (known_folder_list, folder);
		    }
		}
	      /* Something went wrong, destroy the object. */
	      if (status)
		{
		  if (folder->monitor)
		    mu_monitor_destroy (&folder->monitor, folder);
		  if (folder->url)
		    mu_url_destroy (&folder->url);
		  free (folder);
		}
	    }
	  return status;
	}
    }

    return MU_ERR_NOENT;
}

int
mu_folder_create (mu_folder_t *pfolder, const char *name)
{
  return mu_folder_create_from_record (pfolder, name, NULL);
}

/* The folder is destroy if it is the last reference.  */
void
mu_folder_destroy (mu_folder_t *pfolder)
{
  if (pfolder && *pfolder)
    {
      mu_folder_t folder = *pfolder;
      int destroy_lock = 0;
      mu_monitor_t monitor = folder->monitor;

      mu_monitor_wrlock (monitor);

      /* Check if this the last reference for this folder.  If yes removed
         it from the list.  */
      mu_monitor_wrlock (&folder_lock);
      folder->ref--;
      /* Remove the folder from the list of known folder.  */
      if (folder->ref <= 0)
	mu_list_remove (known_folder_list, folder);
      /* If the list is empty we can safely remove it.  */
      if (mu_list_is_empty)
	{
	  mu_list_destroy (&known_folder_list);
	  known_folder_list = NULL;
	}
      mu_monitor_unlock (&folder_lock);

      if (folder->ref <= 0)
	{
	  mu_monitor_unlock (monitor);
	  destroy_lock = 1;
	  /* Notify the observers.  */
	  if (folder->observable)
	    {
	      mu_observable_notify (folder->observable, MU_EVT_FOLDER_DESTROY);
	      mu_observable_destroy (&(folder->observable), folder);
	    }
	  if (folder->_destroy)
	    folder->_destroy (folder);
	  mu_monitor_wrlock (monitor);
	  if (folder->authority)
	    mu_authority_destroy (&(folder->authority), folder);
	  if (folder->stream)
	    mu_stream_destroy (&(folder->stream), folder);
	  if (folder->url)

	    mu_url_destroy (&(folder->url));
	  free (folder);
	}
      mu_monitor_unlock (monitor);
      if (destroy_lock)
	mu_monitor_destroy (&monitor, folder);
      *pfolder = NULL;
    }
}

/* Cover functions.  */
int
mu_folder_open (mu_folder_t folder, int flags)
{
  if (folder == NULL || folder->_open == NULL)
    return ENOSYS;
  return folder->_open (folder, flags);
}

int
mu_folder_close (mu_folder_t folder)
{
  if (folder == NULL || folder->_close == NULL)
    return ENOSYS;
  return folder->_close (folder);
}

int
mu_folder_set_stream (mu_folder_t folder, mu_stream_t stream)
{
  if (folder == NULL)
    return EINVAL;
  if (folder->stream)
    mu_stream_destroy (&(folder->stream), folder);
  folder->stream = stream;
  return 0;
}

int
mu_folder_get_stream (mu_folder_t folder, mu_stream_t *pstream)
{
  if (folder == NULL)
    return EINVAL;
  if (pstream == NULL)
    return MU_ERR_OUT_PTR_NULL;
  *pstream = folder->stream;
  return 0;
}

int
mu_folder_set_authority (mu_folder_t folder, mu_authority_t authority)
{
  if (folder == NULL)
    return EINVAL;
  if (folder->authority)
    mu_authority_destroy (&(folder->authority), folder);
  folder->authority = authority;
  return 0;
}

int
mu_folder_get_authority (mu_folder_t folder, mu_authority_t *pauthority)
{
  if (folder == NULL)
    return EINVAL;
  if (pauthority == NULL)
    return MU_ERR_OUT_PTR_NULL;
  *pauthority = folder->authority;
  return 0;
}

int
mu_folder_get_observable (mu_folder_t folder, mu_observable_t *pobservable)
{
  if (folder == NULL)
    return EINVAL;
  if (pobservable == NULL)
    return MU_ERR_OUT_PTR_NULL;

  if (folder->observable == NULL)
    {
      int status = mu_observable_create (&(folder->observable), folder);
      if (status != 0)
        return status;
    }
  *pobservable = folder->observable;
  return 0;
}

int
mu_folder_has_debug (mu_folder_t folder)
{
  if (folder == NULL)
    return 0;

  return folder->debug ? 1 : 0;
}

int
mu_folder_set_debug (mu_folder_t folder, mu_debug_t debug)
{
  if (folder == NULL)
    return EINVAL;
  if (folder->debug)
    mu_debug_destroy (&(folder->debug), folder);
  folder->debug = debug;
  return 0;
}

int
mu_folder_get_debug (mu_folder_t folder, mu_debug_t *pdebug)
{
  if (folder == NULL)
    return EINVAL;
  if (pdebug == NULL)
    return MU_ERR_OUT_PTR_NULL;
  if (folder->debug == NULL)
    {
      int status = mu_debug_create (&(folder->debug), folder);
      if (status != 0)
        return status;
    }
  *pdebug = folder->debug;
  return 0;
}

void
mu_list_response_free (void *data)
{
  struct mu_list_response *f = data;
  free (f->name);
  free (f);
}

int
mu_folder_list (mu_folder_t folder, const char *dirname, const char *basename,
		size_t max_level,
		mu_list_t *pflist)
{
  int status;
  if (folder == NULL || folder->_list == NULL)
    return EINVAL;
  else
    {
      status = mu_list_create (pflist);
      if (status)
	return status;
      mu_list_set_destroy_item (*pflist, mu_list_response_free);
      status = folder->_list (folder, dirname, basename, max_level, *pflist);
      if (status)
	mu_list_destroy (pflist);
    }
  return status;
}

int
mu_folder_lsub (mu_folder_t folder, const char *dirname, const char *basename,
		mu_list_t *pflist)
{
  int status;
  
  if (folder == NULL || folder->_lsub == NULL)
    return ENOSYS;
  else
    {
      status = mu_list_create (pflist);
      if (status)
	return status;
      mu_list_set_destroy_item (*pflist, mu_list_response_free);
      status = folder->_lsub (folder, dirname, basename, *pflist);
    }
  return status;
}

int
mu_folder_subscribe (mu_folder_t folder, const char *name)
{
  if (folder == NULL || folder->_subscribe == NULL)
    return EINVAL;
  return folder->_subscribe (folder, name);
}

int
mu_folder_unsubscribe (mu_folder_t folder, const char *name)
{
  if (folder == NULL || folder->_unsubscribe == NULL)
    return EINVAL;
  return folder->_unsubscribe (folder, name);
}

int
mu_folder_delete (mu_folder_t folder, const char *name)
{
  if (folder == NULL || folder->_delete == NULL)
    return ENOSYS;
  return folder->_delete (folder, name);
}

int
mu_folder_rename (mu_folder_t folder, const char *oldname, const char *newname)
{
  if (folder == NULL || folder->_rename == NULL)
    return ENOSYS;
  return folder->_rename (folder, oldname, newname);
}

int
mu_folder_get_url (mu_folder_t folder, mu_url_t *purl)
{
  if (folder == NULL)
    return EINVAL;
  if (purl == NULL)
    return MU_ERR_OUT_PTR_NULL;
  *purl = folder->url;
  return 0;
}

static int
is_known_folder (mu_url_t url, mu_folder_t *pfolder)
{
  int ret = 0;
  mu_folder_t folder = NULL;
  mu_iterator_t iterator;

  if (url == NULL || pfolder == NULL)
    return ret;

  if (mu_list_get_iterator (known_folder_list, &iterator) != 0)
    return ret;

  for (mu_iterator_first (iterator); !mu_iterator_is_done (iterator);
       mu_iterator_next (iterator))
    {
      mu_iterator_current (iterator, (void **)&folder);
      /* Check if the same URL type.  */
      if (folder && folder->url
	  && mu_url_is_same_scheme (url, folder->url)
	  && mu_url_is_same_user (url, folder->url)
	  && mu_url_is_same_host (url, folder->url)
	  /*&& mu_url_is_same_path (url, folder->url) */
	  && mu_url_is_same_port (url, folder->url))
	{
	  ret = 1;
	  break;
	}
    }
  if (ret)
    *pfolder = folder;
  mu_iterator_destroy (&iterator);
  return ret;
}

/* Deprecated functions */
static int
list_to_0_6_folder_list (mu_list_t flist, struct mu_0_6_folder_list *pflist)
{
  int status = 0;
  size_t count;
  
  mu_list_count (flist, &count);
  pflist->num = count;
  if (count == 0)
    pflist->element = NULL;
  else
    {
      pflist->element = calloc (count, sizeof (pflist->element[0]));
      if (!pflist->element)
	status = ENOMEM;
      else
	{
	  size_t i;
	  mu_iterator_t itr;
	  
	  mu_list_set_destroy_item (flist, NULL);
	  mu_list_get_iterator (flist, &itr);
	  for (mu_iterator_first (itr), i = 0;
	       !mu_iterator_is_done (itr); mu_iterator_next(itr), i++)
	    mu_iterator_current (itr, (void**) &pflist->element[i]);
	  mu_iterator_destroy (&itr);
	}
    }
  return status;
}

int
mu_0_6_folder_list (mu_folder_t folder,
		    const char *dirname, const char *basename,
		    struct mu_0_6_folder_list *pflist)
{
  mu_list_t flist;
  int status = 0;
  
  status = mu_folder_list (folder, dirname, basename, 0, &flist);
  if (status)
    return status;
  status = list_to_0_6_folder_list (flist, pflist);
  mu_list_destroy (&flist);
  return status;
}

int
mu_0_6_folder_lsub (mu_folder_t folder,
		    const char *dirname, const char *basename,
		    struct mu_0_6_folder_list *pflist)
{
  mu_list_t flist;
  int status = 0;

  status = mu_folder_lsub (folder, dirname, basename, &flist);
  if (status)
    return status;
  status = list_to_0_6_folder_list (flist, pflist);
  mu_list_destroy (&flist);
  return status;
}

int
mu_0_6_folder_list_destroy (struct mu_0_6_folder_list *pflist)
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
