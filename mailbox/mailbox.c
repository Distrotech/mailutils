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

#include <stdlib.h>
#include <errno.h>

#include <mailutils/locker.h>
#include <mailutils/iterator.h>
#include <mailutils/registrar.h>
#include <misc.h>
#include <mailbox0.h>

/* The Mailbox Factory.
   We create an iterator for the mailbox_register and see if any scheme
   match, if not we check in the mailbox_manager register for a match.
   Then we call the mailbox's >url_create() to parse the URL. Last
   initiliaze the concrete mailbox.  */
int
mailbox_create (mailbox_t *pmbox, const char *name, int id)
{
  int status = EINVAL;
  record_t record = NULL;
  mailbox_entry_t entry = NULL;
  iterator_t iterator;
  list_t list;
  int found = 0;

  (void)id;
  if (pmbox == NULL)
    return EINVAL;

  /* Look in the mailbox_register, for a match  */
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
	  status = record_get_mailbox (record, &entry);
	  if (status == 0)
	    found = 1;
	  break;
	}
    }
  iterator_destroy (&iterator);

  if (found)
    {
      url_t url = NULL;
      mailbox_t mbox = NULL;

      /* Allocate memory for mbox.  */
      mbox = calloc (1, sizeof (*mbox));
      if (mbox == NULL)
	return ENOMEM;

      /* Initialize the internal lock, now so the concrete mailbox
	 could use it. */
      status = RWLOCK_INIT (&(mbox->rwlock), NULL);
      if (status != 0)
	{
	  mailbox_destroy (&mbox);
	  return status;
	}

      /* Parse the url, it may be a bad one and we should bailout if this
	 failed.  */
      if ((status = url_create (&url, name)) != 0
	  || (status = entry->_url_init (url)) != 0)
	{
	  mailbox_destroy (&mbox);
	  return status;
	}
      mbox->url = url;

      /* Create the concrete mailbox type.  */
      status = entry->_mailbox_init (mbox);
      if (status != 0)
	{
	  mailbox_destroy (&mbox);
	}
      else
	*pmbox = mbox;
    }
  else
    status = ENOENT;

  return status;
}

void
mailbox_destroy (mailbox_t *pmbox)
{
  if (pmbox && *pmbox)
    {
      mailbox_t mbox = *pmbox;
#ifdef WITH_PTHREAD
      pthread_rwlock_t rwlock = mbox->rwlock;
#endif

      /* Notify the observers.  */
      if (mbox->observable)
	{
	  observable_notify (mbox->observable, MU_EVT_MAILBOX_DESTROY);
	  observable_destroy (&(mbox->observable), mbox);
	}

      /* Call the concrete mailbox.  */
      if (mbox->_destroy)
	mbox->_destroy (mbox);

      RWLOCK_WRLOCK (&(rwlock));

      /* Nuke the stream and close it */
      if (mbox->stream)
	{
	  stream_close (mbox->stream);
	  stream_destroy (&(mbox->stream), mbox);
	}

      if (mbox->authority)
	authority_destroy (&(mbox->authority), mbox);

      if (mbox->url)
        url_destroy (&(mbox->url));

      if (mbox->locker)
	locker_destroy (&(mbox->locker));

      if (mbox->debug)
	debug_destroy (&(mbox->debug), mbox);

      free (mbox);
      *pmbox = NULL;
      RWLOCK_UNLOCK (&(rwlock));
      RWLOCK_DESTROY (&(rwlock));
    }
}


/* -------------- stub functions ------------------- */

int
mailbox_open (mailbox_t mbox, int flag)
{
  if (mbox == NULL || mbox->_open == NULL)
    return ENOSYS;
  return mbox->_open (mbox, flag);
}

int
mailbox_close (mailbox_t mbox)
{
  if (mbox == NULL || mbox->_close == NULL)
    return ENOSYS;
  return mbox->_close (mbox);
}

/* messages */
int
mailbox_append_message (mailbox_t mbox, message_t msg)
{
  if (mbox == NULL || mbox->_append_message == NULL)
    return ENOSYS;
  return mbox->_append_message (mbox, msg);
}

int
mailbox_get_message (mailbox_t mbox, size_t msgno,  message_t *pmsg)
{
  if (mbox == NULL || mbox->_get_message == NULL)
    return ENOSYS;
  return mbox->_get_message (mbox, msgno, pmsg);
}

int
mailbox_messages_count (mailbox_t mbox, size_t *num)
{
  if (mbox == NULL || mbox->_messages_count == NULL)
    return ENOSYS;
  return mbox->_messages_count (mbox, num);
}

int
mailbox_expunge (mailbox_t mbox)
{
  if (mbox == NULL || mbox->_expunge == NULL)
    return ENOSYS;
  return mbox->_expunge (mbox);
}

int
mailbox_is_updated (mailbox_t mbox)
{
  if (mbox == NULL || mbox->_is_updated == NULL)
    return 0;
  return mbox->_is_updated (mbox);
}

int
mailbox_scan (mailbox_t mbox, size_t msgno, size_t *pcount)
{
  if (mbox == NULL || mbox->_scan == NULL)
    return 0;
  return mbox->_scan (mbox, msgno, pcount);
}

int
mailbox_size (mailbox_t mbox, off_t *psize)
{
  if (mbox == NULL || mbox->_size == NULL)
    return 0;
  return mbox->_size (mbox, psize);
}

/* locking */
int
mailbox_set_locker (mailbox_t mbox, locker_t locker)
{
  if (mbox == NULL)
    return EINVAL;
  if (mbox->locker != NULL)
    locker_destroy (&mbox->locker);
  mbox->locker = locker;
  return 0;
}

int
mailbox_get_locker (mailbox_t mbox, locker_t *plocker)
{
  if (mbox == NULL || plocker == NULL)
    return EINVAL;
  if (plocker)
    *plocker = mbox->locker;
  return 0;
}

int
mailbox_set_ticket (mailbox_t mbox, ticket_t ticket)
{
  if (mbox == NULL)
    return EINVAL;
  mbox->ticket = ticket;
  return 0;
}

int
mailbox_get_ticket (mailbox_t mbox, ticket_t *pticket)
{
  if (mbox == NULL || pticket == NULL)
    return EINVAL;
  if (pticket)
    *pticket = mbox->ticket;
  return 0;
}

int
mailbox_set_stream (mailbox_t mbox, stream_t stream)
{
  if (mbox == NULL)
    return EINVAL;
  mbox->stream = stream;
  return 0;
}

int
mailbox_get_stream (mailbox_t mbox, stream_t *pstream)
{
  if (mbox == NULL || pstream == NULL)
    return EINVAL;
  if (pstream)
    *pstream = mbox->stream;
  return 0;
}

int
mailbox_get_observable (mailbox_t mbox, observable_t *pobservable)
{
  if (mbox == NULL  || pobservable == NULL)
    return EINVAL;

  if (mbox->observable == NULL)
    {
      int status = observable_create (&(mbox->observable), mbox);
      if (status != 0)
	return status;
    }
  *pobservable = mbox->observable;
  return 0;
}

int
mailbox_set_debug (mailbox_t mbox, debug_t debug)
{
  if (mbox == NULL)
    return EINVAL;
  debug_destroy (&(mbox->debug), mbox);
  mbox->debug = debug;
  return 0;
}

int
mailbox_get_debug (mailbox_t mbox, debug_t *pdebug)
{
  if (mbox == NULL || pdebug == NULL)
    return EINVAL;
  if (mbox->debug == NULL)
    {
      int status = debug_create (&(mbox->debug), mbox);
      if (status != 0)
	return status;
    }
  *pdebug = mbox->debug;
  return 0;
}

/* Mailbox Internal Locks. Put the name of the functions in parenteses To make
   sure it will not be redefine by a macro.  If the flags was non-blocking we
   should not block on the lock, so we try with pthread_rwlock_try*lock().  */
int
(mailbox_rdlock) (mailbox_t mbox)
{
#ifdef WITH_PTHREAD
  int err = (mbox->flags & MU_STREAM_NONBLOCK) ?
    RWLOCK_TRYRDLOCK (&(mbox->rwlock)) :
    RWLOCK_RDLOCK (&(mbox->rwlock)) ;
  if (err != 0 && err != EDEADLK)
    return err;
#endif
  return 0;
}

int
(mailbox_wrlock) (mailbox_t mbox)
{
#ifdef WITH_PTHREAD
  int err = (mbox->flags & MU_STREAM_NONBLOCK) ?
    RWLOCK_TRYWRLOCK (&(mbox->rwlock)) :
    RWLOCK_WRLOCK (&(mbox->rwlock)) ;
  if (err != 0 && err != EDEADLK)
    return err;
#endif
  return 0;
}

int
(mailbox_unlock) (mailbox_t mbox)
{
#ifdef WITH_PTHREAD
  return RWLOCK_UNLOCK (&(mbox->rwlock));
#else
  return 0;
#endif
}
