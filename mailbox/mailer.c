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
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

#include <mailutils/registrar.h>
#include <mailutils/iterator.h>
#include <misc.h>
#include <mailer0.h>

/*
 */
int
mailer_create (mailer_t *pmailer, const char *name, int id)
{
  int status = EINVAL;
  record_t record = NULL;
  mailer_entry_t entry = NULL;
  list_t list = NULL;
  iterator_t iterator;
  int found = 0;

  (void)id;
  if (pmailer == NULL)
    return EINVAL;

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
          status = record_get_mailer (record, &entry);
          if (status == 0)
            found = 1;
          break;
        }
    }
  iterator_destroy (&iterator);

  if (found)
    {
      url_t url = NULL;
      mailer_t mailer = NULL;

      /* Allocate memory for mailer.  */
      mailer = calloc (1, sizeof (*mailer));
      if (mailer == NULL)
	return ENOMEM;

      RWLOCK_INIT (&(mailer->rwlock), NULL);

      /* Parse the url, it may be a bad one and we should bailout if this
         failed.  */
      if ((status = url_create (&url, name)) != 0
          || (status = entry->_url_init (url)) != 0)
        {
          mailer_destroy (&mailer);
          return status;
        }
      mailer->url = url;

      status = entry->_mailer_init (mailer);
      if (status != 0)
	{
	  mailer_destroy (&mailer);
	}
      else
	*pmailer = mailer;
    }

  return status;
}

void
mailer_destroy (mailer_t *pmailer)
{
  if (pmailer && *pmailer)
    {
      mailer_t mailer = *pmailer;
#ifdef WITH_PTHREAD
      pthread_rwlock_t rwlock = mailer->rwlock;
#endif
      if (mailer->observable)
	{
	  observable_notify (mailer->observable, MU_EVT_MAILER_DESTROY);
	  observable_destroy (&(mailer->observable), mailer);
	}
     /* Call the object.  */
      if (mailer->_destroy)
	mailer->_destroy (mailer);

      RWLOCK_WRLOCK (&rwlock);

      if (mailer->stream)
	{
	  stream_close (mailer->stream);
	  stream_destroy (&(mailer->stream), mailer);
	}
      if (mailer->url)
        url_destroy (&(mailer->url));
      if (mailer->debug)
	debug_destroy (&(mailer->debug), mailer);

      free (mailer);
      *pmailer = NULL;
      RWLOCK_UNLOCK (&rwlock);
      RWLOCK_DESTROY (&rwlock);
    }
}


/* -------------- stub functions ------------------- */

int
mailer_open (mailer_t mailer, int flag)
{
  if (mailer == NULL || mailer->_open == NULL)
    return ENOSYS;
  return mailer->_open (mailer, flag);
}

int
mailer_close (mailer_t mailer)
{
  if (mailer == NULL || mailer->_close == NULL)
    return ENOSYS;
  return mailer->_close (mailer);
}

/* messages */
int
mailer_send_message (mailer_t mailer, message_t msg)
{
  if (mailer == NULL || mailer->_send_message == NULL)
    return ENOSYS;
  return mailer->_send_message (mailer, msg);
}

int
mailer_set_stream (mailer_t mailer, stream_t stream)
{
  if (mailer == NULL)
    return EINVAL;
  mailer->stream = stream;
  return 0;
}

int
mailer_get_stream (mailer_t mailer, stream_t *pstream)
{
  if (mailer == NULL || pstream == NULL)
    return EINVAL;
  if (pstream)
    *pstream = mailer->stream;
  return 0;
}

int
mailer_get_observable (mailer_t mailer, observable_t *pobservable)
{
  /* FIXME: I should check for invalid types */
  if (mailer == NULL || pobservable == NULL)
    return EINVAL;

  if (mailer->observable == NULL)
    {
      int status = observable_create (&(mailer->observable), mailer);
      if (status != 0)
	return status;
    }
  *pobservable = mailer->observable;
  return 0;
}

int
mailer_set_debug (mailer_t mailer, debug_t debug)
{
  if (mailer == NULL)
    return EINVAL;
  debug_destroy (&(mailer->debug), mailer);
  mailer->debug = debug;
  return 0;
}

int
mailer_get_debug (mailer_t mailer, debug_t *pdebug)
{
  if (mailer == NULL || pdebug == NULL)
    return EINVAL;
  if (mailer->debug == NULL)
    {
      int status = debug_create (&(mailer->debug), mailer);
      if (status != 0)
	return status;
    }
  *pdebug = mailer->debug;
  return 0;
}

/* Mailer Internal Locks. Put the name of the functions in parenteses To make
   they will not be redefine by the macro.  If the flags was non-blocking we
   should not block on the lock, so we try with pthread_rwlock_try*lock().  */
int
(mailer_wrlock) (mailer_t mailer)
{
#ifdef WITH_PTHREAD
  int err = (mailer->flags & MU_STREAM_NONBLOCK) ?
    RWLOCK_TRYWRLOCK (&(mailer->rwlock)) :
    RWLOCK_WRLOCK (&(mailer->rwlock)) ;
  if (err != 0 && err != EDEADLK)
    return err;
#endif
  return 0;
}
int
(mailer_rdlock) (mailer_t mailer)
{
#ifdef WITH_PTHREAD
  int err = (mailer->flags & MU_STREAM_NONBLOCK) ?
    RWLOCK_TRYRDLOCK (&(mailer->rwlock)) :
    RWLOCK_RDLOCK (&(mailer->rwlock)) ;
  if (err != 0 && err != EDEADLK)
    return err;
#endif
  return 0;
}

int
(mailer_unlock) (mailer_t mailer)
{
#ifdef WITH_PTHREAD
  return RWLOCK_UNLOCK (&(mailer->rwlock));
#else
  return 0;
#endif
}
