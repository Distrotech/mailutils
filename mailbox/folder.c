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
#include <string.h>
#ifdef HAVE_PTHREAD_H
#  include <pthread.h>
#endif

#include <mailutils/registrar.h>
#include <mailutils/iterator.h>
#include <mailutils/list.h>

#include <misc.h>
#include <folder0.h>

#ifdef WITH_PTHREAD
static pthread_mutex_t slock = PTHREAD_MUTEX_INITIALIZER;
#endif

/* Internal folder list.  */
static list_t known_folder_list;
static int is_known_folder (url_t, folder_t *);
static int is_same_scheme (url_t, url_t);
static int is_same_user (url_t, url_t);
static int is_same_path (url_t, url_t);
static int is_same_host (url_t, url_t);
static int is_same_port (url_t, url_t);

/*  A folder could be a remote one, IMAP, or local, a spool directory
    like $HOME/Mail etc ..  We maintain a known list of folder to
    not generate multiple folder of the same URL.  Meaning when
    folder_create () is call we'll check if we already have a
    folder for that URL and return the same, if not we create a new one.
    The downside, the algo to detect the same URL is very weak, and
    they maybe cases where you want a different folder for the same
    URL, there is not easy way to do this.  */
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

  /* Look in the registrar list(iterator), for a match  */
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

      /* Parse the url, it may be a bad one and we should bailout if this
	 failed.  */
      if ((status = url_create (&url, name) != 0)
          || (status = entry->_url_init (url)) != 0)
	return status;

#ifdef WITH_PTHREAD
      pthread_mutex_lock (&slock);
#endif
      /* Check if we already have the same URL folder.  */
      if (is_known_folder (url, &folder))
	{
	  folder->ref++;
	  *pfolder = folder;
	  url_destroy (&url);
#ifdef WITH_PTHREAD
	  pthread_mutex_unlock (&slock);
#endif
	  return  0;
	}
#ifdef WITH_PTHREAD
      else
	pthread_mutex_unlock (&slock);
#endif

      /* Create a new folder.  */

      /* Allocate memory for folder.  */
      folder = calloc (1, sizeof (*folder));
      if (folder != NULL)
	{
	  folder->url = url;
	  /* Initialize the internal lock, now so the concrete
	     folder could use it.  */
	  status = monitor_create (&(folder->monitor), folder);
	  if (status == 0)
	    {
	      /* Create the concrete folder type.  */
	      status = entry->_folder_init (folder);
	      if (status == 0)
		{
		  *pfolder = folder;
		  folder->ref++;
		  /* Put on the internal list of known folders.  */
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

void
folder_destroy (folder_t *pfolder)
{
  if (pfolder && *pfolder)
    {
      folder_t folder = *pfolder;
      int destroy_lock = 0;
      monitor_t monitor = folder->monitor;

      monitor_wrlock (monitor);
#ifdef WITH_PTHREAD
      pthread_mutex_lock (&slock);
#endif
      folder->ref--;
      /* Remove the folder from the list of known folder.  */
      if (folder->ref <= 0)
	list_remove (known_folder_list, folder);
#ifdef WITH_PHTREAD
      pthread_mutex_unlock (&slock);
#endif
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
folder_list (folder_t folder, char *vector[][], size_t *pnum)
{
  if (folder == NULL || folder->_list == NULL)
    return ENOSYS;
  return folder->_list (folder, vector, pnum);
}

int
folder_delete_mailbox (folder_t folder, const char *name)
{
  if (folder == NULL || folder->_delete_mailbox == NULL)
    return ENOSYS;
  return folder->_delete_mailbox (folder, name);
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
	  && is_same_scheme (url, folder->url)
	  && is_same_user (url, folder->url)
	  && is_same_path (url, folder->url)
	  && is_same_host (url, folder->url)
	  && is_same_port (url, folder->url))
	{
	  ret = 1;
	  break;
	}
    }
  iterator_destroy (&iterator);
  return ret;
}

static int
is_same_scheme (url_t url1, url_t url2)
{
  size_t i = 0, j = 0;
  char *s1, *s2;
  int ret = 1;

  url_get_scheme (url1, NULL, 0, &i);
  url_get_scheme (url2, NULL, 0, &j);
  s1 = calloc (i + 1, sizeof (char));
  if (s1)
    {
      url_get_scheme (url1, s1, i + 1, NULL);
      s2 = calloc (j + 1, sizeof (char));
      if (s2)
	{
	  url_get_scheme (url2, s2, j + 1, NULL);
	  ret = !strcasecmp (s1, s2);
	  free (s2);
	}
      free (s1);
    }
  return ret;
}

static int
is_same_user (url_t url1, url_t url2)
{
  size_t i = 0, j = 0;
  char *s1, *s2;
  int ret = 0;

  url_get_user (url1, NULL, 0, &i);
  url_get_user (url2, NULL, 0, &j);
  s1 = calloc (i + 1, sizeof (char));
  if (s1)
    {
      url_get_user (url1, s1, i + 1, NULL);
      s2 = calloc (j + 1, sizeof (char));
      if (s2)
	{
	  url_get_user (url2, s2, j + 1, NULL);
	  ret = !strcasecmp (s1, s2);
	  free (s2);
	}
      free (s1);
    }
  return ret;
}

static int
is_same_path (url_t url1, url_t url2)
{
  size_t i = 0, j = 0;
  char *s1, *s2;
  int ret = 0;

  url_get_path (url1, NULL, 0, &i);
  url_get_path (url2, NULL, 0, &j);
  s1 = calloc (i + 1, sizeof (char));
  if (s1)
    {
      url_get_path (url1, s1, i + 1, NULL);
      s2 = calloc (j + 1, sizeof (char));
      if (s2)
	{
	  url_get_path (url2, s2, j + 1, NULL);
	  ret = !strcasecmp (s1, s2);
	  free (s2);
	}
      free (s1);
    }
  return ret;
}

static int
is_same_host (url_t url1, url_t url2)
{
  size_t i = 0, j = 0;
  char *s1, *s2;
  int ret = 0;

  url_get_host (url1, NULL, 0, &i);
  url_get_host (url2, NULL, 0, &j);
  s1 = calloc (i + 1, sizeof (char));
  if (s1)
    {
      url_get_host (url1, s1, i + 1, NULL);
      s2 = calloc (j + 1, sizeof (char));
      if (s2)
	{
	  url_get_host (url2, s2, j + 1, NULL);
	  ret = !strcasecmp (s1, s2);
	  free (s2);
	}
      free (s1);
    }
  return ret;
}

static int
is_same_port (url_t url1, url_t url2)
{
  long p1 = 0, p2 = 0;

  url_get_port (url1, &p1);
  url_get_port (url2, &p2);
  return (p1 == p2);
}
