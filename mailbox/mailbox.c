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
#include <config.h>
#endif

#include <stdlib.h>
#include <errno.h>

#include <mailutils/errno.h>
#include <mailutils/error.h>
#include <mailutils/iterator.h>
#include <mailutils/registrar.h>
#include <mailbox0.h>

/* The Mailbox Factory.
   Create an iterator for registrar and see if any url scheme match,
   Then we call the mailbox's url_create() to parse the URL. Last
   initialize the concrete mailbox and folder.  */
int
mailbox_create (mailbox_t *pmbox, const char *name)
{
  int status = EINVAL;
  record_t record = NULL;
  int (*m_init) __P ((mailbox_t)) = NULL;
  int (*u_init) __P ((url_t)) = NULL;
  iterator_t iterator;
  list_t list;
  int found = 0;

  if (pmbox == NULL)
    return MU_ERR_OUT_PTR_NULL;

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
	  record_get_mailbox (record, &m_init);
	  record_get_url (record, &u_init);
	  if (m_init && u_init)
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
      mailbox_t mbox;

      /* Allocate memory for mbox.  */
      mbox = calloc (1, sizeof (*mbox));
      if (mbox == NULL)
	return ENOMEM;

      /* Initialize the internal lock now, so the concrete mailbox
	 could use it. */
      status = monitor_create (&(mbox->monitor), 0, mbox);
      if (status != 0)
	{
	  mailbox_destroy (&mbox);
	  return status;
	}

      /* Parse the url, it may be a bad one and we should bailout if this
	 failed.  */
      if ((status = url_create (&url, name)) != 0
	  || (status = u_init (url)) != 0)
	{
	  mailbox_destroy (&mbox);
	  return status;
	}
      mbox->url = url;

      /* Create the folder before initializing the concrete mailbox.
	 The mailbox needs it's back pointer. */
      {
	folder_t folder;
	if (folder_create (&folder, name) == 0)
	  mbox->folder = folder;
      }

      /* Create the concrete mailbox type.  */
      status = m_init (mbox);
      if (status != 0)
	{
	  mailbox_destroy (&mbox);
	}
      else
	{
	  *pmbox = mbox;
	}

    }
  else
    status = MU_ERR_NO_HANDLER;

  return status;
}

void
mailbox_destroy (mailbox_t *pmbox)
{
  if (pmbox && *pmbox)
    {
      mailbox_t mbox = *pmbox;
      monitor_t monitor = mbox->monitor;

      /* Notify the observers.  */
      if (mbox->observable)
	{
	  observable_notify (mbox->observable, MU_EVT_MAILBOX_DESTROY);
	  observable_destroy (&(mbox->observable), mbox);
	}

      /* Call the concrete mailbox _destroy method. So it can clean itself.  */
      if (mbox->_destroy)
	mbox->_destroy (mbox);

      monitor_wrlock (monitor);

      /* Close the stream and nuke it */
      if (mbox->stream)
	{
	  /* FIXME:  Is this right, should be the client responsabilty to close
	     the stream?  */
	  /* stream_close (mbox->stream); */
	  stream_destroy (&(mbox->stream), mbox);
	}

      if (mbox->url)
        url_destroy (&(mbox->url));

      if (mbox->locker)
	locker_destroy (&(mbox->locker));

      if (mbox->debug)
	mu_debug_destroy (&(mbox->debug), mbox);

      if (mbox->folder)
	folder_destroy (&(mbox->folder));

      if (mbox->property)
	property_destroy (&(mbox->property), mbox);

      free (mbox);
      *pmbox = NULL;
      monitor_unlock (monitor);
      monitor_destroy (&monitor, mbox);
    }
}


/* -------------- stub functions ------------------- */

int
mailbox_open (mailbox_t mbox, int flag)
{
  if (mbox == NULL || mbox->_open == NULL)
    return MU_ERR_EMPTY_VFN;
  return mbox->_open (mbox, flag);
}

int
mailbox_close (mailbox_t mbox)
{
  if (mbox == NULL || mbox->_close == NULL)
    return MU_ERR_EMPTY_VFN;
  return mbox->_close (mbox);
}

/* messages */
int
mailbox_append_message (mailbox_t mbox, message_t msg)
{
  if (mbox == NULL || mbox->_append_message == NULL)
    return MU_ERR_EMPTY_VFN;
  return mbox->_append_message (mbox, msg);
}

int
mailbox_get_message (mailbox_t mbox, size_t msgno,  message_t *pmsg)
{
  if (mbox == NULL || mbox->_get_message == NULL)
    return MU_ERR_EMPTY_VFN;
  return mbox->_get_message (mbox, msgno, pmsg);
}

int
mailbox_messages_count (mailbox_t mbox, size_t *num)
{
  if (mbox == NULL || mbox->_messages_count == NULL)
    return MU_ERR_EMPTY_VFN;
  return mbox->_messages_count (mbox, num);
}

int
mailbox_messages_recent (mailbox_t mbox, size_t *num)
{
  if (mbox == NULL || mbox->_messages_recent == NULL)
    return MU_ERR_EMPTY_VFN;
  return mbox->_messages_recent (mbox, num);
}
int
mailbox_message_unseen (mailbox_t mbox, size_t *num)
{
  if (mbox == NULL || mbox->_message_unseen == NULL)
    return MU_ERR_EMPTY_VFN;
  return mbox->_message_unseen (mbox, num);
}

int
mailbox_save_attributes (mailbox_t mbox)
{
  if (mbox == NULL || mbox->_save_attributes == NULL)
    return MU_ERR_EMPTY_VFN;
  return mbox->_save_attributes (mbox);
}

int
mailbox_expunge (mailbox_t mbox)
{
  if (mbox == NULL || mbox->_expunge == NULL)
    return MU_ERR_EMPTY_VFN;
  return mbox->_expunge (mbox);
}

int
mailbox_is_updated (mailbox_t mbox)
{
  if (mbox == NULL || mbox->_is_updated == NULL)
    return 1;
  return mbox->_is_updated (mbox);
}

int
mailbox_scan (mailbox_t mbox, size_t msgno, size_t *pcount)
{
  if (mbox == NULL || mbox->_scan == NULL)
    return MU_ERR_EMPTY_VFN;
  return mbox->_scan (mbox, msgno, pcount);
}

int
mailbox_get_size (mailbox_t mbox, off_t *psize)
{
  if (mbox == NULL || mbox->_get_size == NULL)
    return MU_ERR_EMPTY_VFN;
  return mbox->_get_size (mbox, psize);
}

int
mailbox_uidvalidity (mailbox_t mbox, unsigned long *pvalid)
{
  if (mbox == NULL || mbox->_uidvalidity == NULL)
    return MU_ERR_EMPTY_VFN;
  return mbox->_uidvalidity (mbox, pvalid);
}

int
mailbox_uidnext (mailbox_t mbox, size_t *puidnext)
{
  if (mbox == NULL || mbox->_uidnext == NULL)
    return MU_ERR_EMPTY_VFN;
  return mbox->_uidnext (mbox, puidnext);
}

/* locking */
int
mailbox_set_locker (mailbox_t mbox, locker_t locker)
{
  if (mbox == NULL)
    return MU_ERR_MBX_NULL;
  if (mbox->locker)
    locker_destroy (&mbox->locker);
  mbox->locker = locker;
  return 0;
}

int
mailbox_get_locker (mailbox_t mbox, locker_t *plocker)
{
  if (mbox == NULL)
    return MU_ERR_MBX_NULL;
  if (plocker == NULL)
    return MU_ERR_OUT_PTR_NULL;
  *plocker = mbox->locker;
  return 0;
}

int
mailbox_set_stream (mailbox_t mbox, stream_t stream)
{
  if (mbox == NULL)
    return MU_ERR_MBX_NULL;
  if (mbox->stream)
    stream_destroy (&(mbox->stream), mbox);
  mbox->stream = stream;
  return 0;
}

/* FIXME: This is a problem.  We provide a mailbox_get_stream ()
   and this stream is special it should, in theory, represent
   a "view" of a flow of messages.  But providing this perspective
   may make sense for local mailboxes but downright impossible
   for a remote mailbox, short on downloading the entire mailbox
   locally.
   The question is : should this function be removed?
   So far it as been used on local mailboxes to get offsets.  */
int
mailbox_get_stream (mailbox_t mbox, stream_t *pstream)
{
  if (mbox == NULL)
    return MU_ERR_MBX_NULL;
  if (pstream == NULL)
    return MU_ERR_OUT_PTR_NULL;

  /* If null two cases:
     - it is no open yet.
     - it a remote stream and the socket stream is on the folder.  */
  if (mbox->stream == NULL)
    {
      if (mbox->folder)
	return folder_get_stream (mbox->folder, pstream);
    }
  *pstream = mbox->stream;
  return 0;
}

int
mailbox_get_observable (mailbox_t mbox, observable_t *pobservable)
{
  if (mbox == NULL)
    return MU_ERR_MBX_NULL;
  if (pobservable == NULL)
    return MU_ERR_OUT_PTR_NULL;

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
mailbox_get_property (mailbox_t mbox, property_t *pproperty)
{
  if (mbox == NULL)
    return MU_ERR_MBX_NULL;
  if (pproperty == NULL)
    return MU_ERR_OUT_PTR_NULL;
  
  if (mbox->property == NULL)
    {
      int status = property_create (&(mbox->property), mbox);
      if (status != 0)
	return status;
    }
  *pproperty = mbox->property;
  return 0;
}

int
mailbox_has_debug (mailbox_t mailbox)
{
  if (mailbox == NULL)
    return 0;

  return mailbox->debug ? 1 : 0;
}

int
mailbox_set_debug (mailbox_t mbox, mu_debug_t debug)
{
  if (mbox == NULL)
    return MU_ERR_MBX_NULL;
  if (mbox->debug)
    mu_debug_destroy (&mbox->debug, mbox);
  mbox->debug = debug;
  if(!folder_has_debug(mbox->folder))
    folder_set_debug(mbox->folder, debug);
  return 0;
}

int
mailbox_get_debug (mailbox_t mbox, mu_debug_t *pdebug)
{
  if (mbox == NULL)
    return MU_ERR_MBX_NULL;
  if (pdebug == NULL)
    return MU_ERR_OUT_PTR_NULL;
  if (mbox->debug == NULL)
    {
      int status = mu_debug_create (&(mbox->debug), mbox);
      if (status != 0)
	return status;
    }
  *pdebug = mbox->debug;
  return 0;
}

int
mailbox_get_url (mailbox_t mbox, url_t *purl)
{
  if (mbox == NULL)
    return MU_ERR_MBX_NULL;
  if (purl == NULL)
    return MU_ERR_OUT_PTR_NULL;
  *purl = mbox->url;
  return 0;
}

int
mailbox_get_folder (mailbox_t mbox, folder_t *pfolder)
{
  if (mbox == NULL)
    return MU_ERR_MBX_NULL;
  if (pfolder == NULL)
    return MU_ERR_OUT_PTR_NULL;
  *pfolder = mbox->folder;
  return 0;
}

int
mailbox_set_folder (mailbox_t mbox, folder_t folder)
{
  if (mbox == NULL)
    return MU_ERR_MBX_NULL;
   mbox->folder = folder;
  return 0;
}
