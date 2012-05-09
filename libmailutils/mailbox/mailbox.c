/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2001, 2004-2012 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library.  If not, see 
   <http://www.gnu.org/licenses/>. */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <mailutils/debug.h>
#include <mailutils/errno.h>
#include <mailutils/error.h>
#include <mailutils/folder.h>
#include <mailutils/iterator.h>
#include <mailutils/list.h>
#include <mailutils/locker.h>
#include <mailutils/observer.h>
#include <mailutils/property.h>
#include <mailutils/registrar.h>
#include <mailutils/stream.h>
#include <mailutils/url.h>
#include <mailutils/attribute.h>
#include <mailutils/message.h>
#include <mailutils/util.h>

#include <mailutils/sys/mailbox.h>
#include <mailutils/sys/folder.h>
#include <mailutils/sys/url.h>

/* Mailbox-specific flags */
#define _MU_MAILBOX_OPEN    0x10000000
#define _MU_MAILBOX_REMOVED 0x20000000
#define _MU_MAILBOX_MASK    0xF0000000

static int
mailbox_folder_create (mu_mailbox_t mbox, const char *name,
		       mu_record_t record)
{
  int rc;
  mu_url_t url;
  
  if ((rc = mu_url_uplevel (mbox->url, &url)))
    {
      if (rc == MU_ERR_NOENT)
	{
	  rc = mu_url_dup (mbox->url, &url);
	  if (rc)
	    return rc;
	}
      else
	return rc;
    }

  rc = mu_folder_create_from_record (&mbox->folder, url, record);
  if (rc)
    mu_url_destroy (&url);
  return rc;
}

int
_mailbox_create_from_record (mu_mailbox_t *pmbox,
			     mu_record_t record,
			     mu_url_t url,
			     mu_folder_t folder,
			     const char *name)
{
  int (*m_init) (mu_mailbox_t) = NULL;
      
  mu_record_get_mailbox (record, &m_init);
  if (m_init)
    {
      int status;
      int (*u_init) (mu_url_t) = NULL;
      mu_mailbox_t mbox;
      
      /* Allocate memory for mbox.  */
      mbox = calloc (1, sizeof (*mbox));
      if (mbox == NULL)
	return ENOMEM;
      
      /* Initialize the internal lock now, so the concrete mailbox
	 could use it. */
      status = mu_monitor_create (&mbox->monitor, 0, mbox);
      if (status != 0)
	{
	  mu_mailbox_destroy (&mbox);
	  return status;
	}
      
      /* Make sure scheme contains actual mailbox scheme */
      /* FIXME: It is appropriate not for all record types.  For now we
	 assume that if the record scheme ends with a plus sign, this
	 should not be done.  Probably it requires some flag in struct
	 _mu_record? */
      if (strcmp (url->scheme, record->scheme))
	{
	  char *p = strdup (record->scheme);
	  if (!p)
	    {
	      mu_mailbox_destroy (&mbox);
	      return errno;
	    }
	  free (url->scheme);
	  url->scheme = p;
	}
      
      mu_record_get_url (record, &u_init);
      if (u_init && (status = u_init (url)) != 0)
	{
	  mu_mailbox_destroy (&mbox);
	  return status;
	}
      
      mbox->url = url;

      if (folder)
	{
	  folder->ref++; /* FIXME: No ref/unref function for folders */
	  mbox->folder = folder;
	}
      else
	/* Create the folder before initializing the concrete mailbox.
	   The mailbox needs it's back pointer. */
	status = mailbox_folder_create (mbox, name, record);
      
      if (status == 0)
	status = m_init (mbox);   /* Create the concrete mailbox type.  */
      
      if (status != 0)
	{
	  /* Take care not to destroy url.  Leave it to caller. */
	  mbox->url = NULL;
	  mu_mailbox_destroy (&mbox);
	}
      else
	*pmbox = mbox;
      return status;
    }
  return ENOSYS;
}

static int
_create_mailbox0 (mu_mailbox_t *pmbox, mu_url_t url, const char *name)
{
  mu_record_t record = NULL;
  int rc;
  
  rc = mu_registrar_lookup_url (url, MU_FOLDER_ATTRIBUTE_FILE, &record, NULL);
  if (rc == 0)
    rc = _mailbox_create_from_record (pmbox, record, url, NULL, name);
  return rc;
}

static int
_create_mailbox (mu_mailbox_t *pmbox, const char *name)
{
  int status;
  mu_url_t url;

  status = mu_url_create (&url, name);
  if (status)
    return status;
  status = _create_mailbox0 (pmbox, url, name);
  if (status)
    mu_url_destroy (&url);
  return status;
}

/* The Mailbox Factory.
 */
int
mu_mailbox_create (mu_mailbox_t *pmbox, const char *name)
{
  if (pmbox == NULL)
    return MU_ERR_OUT_PTR_NULL;

  return _create_mailbox (pmbox, name);
}

int
mu_mailbox_create_from_url (mu_mailbox_t *pmbox, mu_url_t url)
{
  if (pmbox == NULL)
    return MU_ERR_OUT_PTR_NULL;
  return _create_mailbox0 (pmbox, url, mu_url_to_string (url));
}

int
mu_mailbox_create_from_record (mu_mailbox_t *pmbox, mu_record_t record,
			       const char *name)
{
  mu_url_t url;
  int rc;
  
  rc = mu_url_create (&url, name);
  if (rc)
    return rc;
  rc = _mailbox_create_from_record (pmbox, record, url, NULL, name);
  if (rc)
    mu_url_destroy (&url);
  return rc;
}

int
mu_mailbox_create_at (mu_mailbox_t *pmbox, mu_folder_t folder,
		      const char *name)
{
  int rc;
  mu_url_t url;
  const char *oldpath;
  
  rc = mu_url_dup (folder->url, &url);
  if (rc)
    return rc;
  do
    {
      char *path;
      size_t oldlen, len;
      mu_record_t record;
      
      rc = mu_url_sget_path (url, &oldpath);
      if (rc)
	break;

      oldlen = strlen (oldpath);
      if (oldlen == 0)
	{
	  path = strdup (name);
	  if (!path)
	    {
	      rc = ENOMEM;
	      break;
	    }
	}
      else
	{
	  if (oldpath[oldlen-1] == '/')
	    oldlen--;
	  len = oldlen + 1 + strlen (name) + 1;
	  path = malloc (len);
	  if (!path)
	    {
	      rc = ENOMEM;
	      break;
	    }
	  memcpy (path, oldpath, oldlen);
	  path[oldlen++] = '/';
	  strcpy (path + oldlen, name);
	}
      rc = mu_url_set_path (url, path);
      free (path);
      if (rc)
	break;

      rc = mu_registrar_lookup_url (url, MU_FOLDER_ATTRIBUTE_FILE,
				    &record, NULL);
      if (rc)
	break;
      rc = _mailbox_create_from_record (pmbox, record, url, folder, name);
    }
  while (0);

  if (rc)
    mu_url_destroy (&url);
  return rc;
}

void
mu_mailbox_destroy (mu_mailbox_t *pmbox)
{
  if (pmbox && *pmbox)
    {
      mu_mailbox_t mbox = *pmbox;
      mu_monitor_t monitor = mbox->monitor;

      /* Notify the observers.  */
      if (mbox->observable)
	{
	  mu_observable_notify (mbox->observable, MU_EVT_MAILBOX_DESTROY,
				mbox);
	  mu_observable_destroy (&mbox->observable, mbox);
	}

      /* Call the concrete mailbox _destroy method. So it can clean itself.  */
      if (mbox->_destroy)
	mbox->_destroy (mbox);

      mu_monitor_wrlock (monitor);

      /* Close the stream and nuke it */
      if (mbox->stream)
	{
	  /* FIXME: Is this right, should the client be responsible
	     for closing the stream?  */
	  /* mu_stream_close (mbox->stream); */
	  mu_stream_destroy (&mbox->stream);
	}

      if (mbox->url)
        mu_url_destroy (&mbox->url);

      if (mbox->locker)
	mu_locker_destroy (&mbox->locker);

      if (mbox->folder)
	mu_folder_destroy (&mbox->folder);

      if (mbox->property)
	mu_property_destroy (&mbox->property);

      free (mbox);
      *pmbox = NULL;
      mu_monitor_unlock (monitor);
      mu_monitor_destroy (&monitor, mbox);
    }
}


/* -------------- stub functions ------------------- */

int
mu_mailbox_open (mu_mailbox_t mbox, int flag)
{
  int rc;
  
  if (!mbox)
    return EINVAL;
  if (mbox->_open == NULL)
    return MU_ERR_EMPTY_VFN;
  if (mbox->flags & _MU_MAILBOX_OPEN)
    return MU_ERR_OPEN;
  if (flag & MU_STREAM_QACCESS)
    {
      /* Quick access mailboxes are read-only */
      if (flag & (MU_STREAM_WRITE 
		  | MU_STREAM_APPEND | MU_STREAM_CREAT))
	return EACCES;
    }
  rc = mbox->_open (mbox, flag);
  if (rc == 0)
    mbox->flags |= _MU_MAILBOX_OPEN;
  return rc;
}

int
mu_mailbox_close (mu_mailbox_t mbox)
{
  int rc;

  if (!mbox)
    return EINVAL;
  if (!(mbox->flags & _MU_MAILBOX_OPEN))
    return MU_ERR_NOT_OPEN;
  if (mbox == NULL || mbox->_close == NULL)
    return MU_ERR_EMPTY_VFN;

  rc = mbox->_close (mbox);
  if (rc == 0)
    mbox->flags &= ~_MU_MAILBOX_OPEN;
  return rc;
}

int
mu_mailbox_remove (mu_mailbox_t mbox)
{
  if (!mbox)
    return EINVAL;
  if (mbox->flags & _MU_MAILBOX_OPEN)
    return MU_ERR_OPEN;
  if (mbox->flags & _MU_MAILBOX_REMOVED)
    return MU_ERR_MBX_REMOVED;
  if (!mbox->_remove)
    {
      /* Try the owning folder delete method.  See comment to mu_folder_delete
	 in folder.c.  This may result in a recursive call to mu_mailbox_remove
	 which is blocked by setting the _MU_MAILBOX_REMOVED flag. */

      int rc;
      const char *path;
      
      rc = mu_url_sget_path (mbox->url, &path);
      if (rc == 0)
	{
	  mbox->flags |= _MU_MAILBOX_REMOVED;
	  rc = mu_folder_delete (mbox->folder, path);
	  if (rc)
	    mbox->flags &= ~_MU_MAILBOX_REMOVED;
	}
      return rc;
    }
  return mbox->_remove (mbox);
}

int
mu_mailbox_flush (mu_mailbox_t mbox, int expunge)
{
  size_t i, total = 0;
  int status = 0;
  
  if (!mbox)
    return EINVAL;
  if (mbox->flags & _MU_MAILBOX_REMOVED)
    return MU_ERR_MBX_REMOVED;
  if (!(mbox->flags & _MU_MAILBOX_OPEN))
    return _MU_MAILBOX_OPEN;
  if (!(mbox->flags & (MU_STREAM_WRITE|MU_STREAM_APPEND)))
    return 0;

  mu_mailbox_messages_count (mbox, &total);
  if (!(mbox->flags & MU_STREAM_APPEND))
    for (i = 1; i <= total; i++)
      {
	mu_message_t msg = NULL;
	mu_attribute_t attr = NULL;
	mu_mailbox_get_message (mbox, i, &msg);
	mu_message_get_attribute (msg, &attr);
	mu_attribute_set_seen (attr);
      }

  if (expunge)
    status = mu_mailbox_expunge (mbox);
  else
    status = mu_mailbox_sync (mbox);

  return status;
}

#define _MBOX_CHECK_FLAGS(mbox)			\
  if (mbox == NULL)				\
    return EINVAL;			        \
  if (mbox->flags & _MU_MAILBOX_REMOVED)	\
    return MU_ERR_MBX_REMOVED;			\
  if (!(mbox->flags & _MU_MAILBOX_OPEN))	\
    return MU_ERR_NOT_OPEN

#define _MBOX_CHECK(mbox,method)		\
  _MBOX_CHECK_FLAGS(mbox);			\
  if (mbox->method == NULL)			\
    return MU_ERR_EMPTY_VFN

#define _MBOX_CHECK_Q(mbox,method)		\
  _MBOX_CHECK(mbox,method);			\
  if (mbox->flags & MU_STREAM_QACCESS)		\
    return MU_ERR_BADOP

/* messages */
int
mu_mailbox_append_message (mu_mailbox_t mbox, mu_message_t msg)
{
  _MBOX_CHECK_Q (mbox, _append_message);
  if (!(mbox->flags & (MU_STREAM_WRITE|MU_STREAM_APPEND)))
    return EACCES;
  return mbox->_append_message (mbox, msg);
}

int
mu_mailbox_get_message (mu_mailbox_t mbox, size_t msgno,  mu_message_t *pmsg)
{
  _MBOX_CHECK_Q (mbox, _get_message);
  return mbox->_get_message (mbox, msgno, pmsg);
}

int
mu_mailbox_quick_get_message (mu_mailbox_t mbox, mu_message_qid_t qid,
			      mu_message_t *pmsg)
{
  _MBOX_CHECK (mbox, _quick_get_message);
  if (!(mbox->flags & MU_STREAM_QACCESS))
    return MU_ERR_BADOP;
  return mbox->_quick_get_message (mbox, qid, pmsg);
}

int
mu_mailbox_messages_count (mu_mailbox_t mbox, size_t *num)
{
  _MBOX_CHECK_Q (mbox, _messages_count);
  return mbox->_messages_count (mbox, num);
}

int
mu_mailbox_messages_recent (mu_mailbox_t mbox, size_t *num)
{
  _MBOX_CHECK_Q (mbox, _messages_recent);
  return mbox->_messages_recent (mbox, num);
}

int
mu_mailbox_message_unseen (mu_mailbox_t mbox, size_t *num)
{
  _MBOX_CHECK_Q (mbox, _message_unseen);
  return mbox->_message_unseen (mbox, num);
}

int
mu_mailbox_sync (mu_mailbox_t mbox)
{
  _MBOX_CHECK_Q (mbox, _sync);
  if (!(mbox->flags & (MU_STREAM_WRITE|MU_STREAM_APPEND)))
    return 0;
  return mbox->_sync (mbox);
}

/* Historic alias: */
int
mu_mailbox_save_attributes (mu_mailbox_t mbox)
{
  return mu_mailbox_sync (mbox);
}

int
mu_mailbox_expunge (mu_mailbox_t mbox)
{
  _MBOX_CHECK_Q (mbox, _expunge);
  if (!(mbox->flags & (MU_STREAM_WRITE|MU_STREAM_APPEND)))
    return EACCES;
  return mbox->_expunge (mbox);
}

int
mu_mailbox_is_updated (mu_mailbox_t mbox)
{
  if (mbox == NULL ||
      !(mbox->flags & _MU_MAILBOX_OPEN) ||
      (mbox->flags & _MU_MAILBOX_REMOVED) ||
      mbox->_is_updated == NULL)
    return 1;
  if (mbox->flags & MU_STREAM_QACCESS)
    return 1;
  return mbox->_is_updated (mbox);
}

int
mu_mailbox_scan (mu_mailbox_t mbox, size_t msgno, size_t *pcount)
{
  _MBOX_CHECK_Q (mbox, _scan);
  return mbox->_scan (mbox, msgno, pcount);
}

int
mu_mailbox_get_size (mu_mailbox_t mbox, mu_off_t *psize)
{
  int status;

  _MBOX_CHECK_FLAGS (mbox);
  if (mbox->flags & MU_STREAM_QACCESS)
    return MU_ERR_BADOP;
  if (mbox->_get_size == NULL
      || (status = mbox->_get_size (mbox, psize)) == ENOSYS)
    {
      /* Fall back to brute-force method */
      size_t i, total;
      mu_off_t size = 0;
      
      status = mu_mailbox_messages_count (mbox, &total);
      if (status)
	return status;
      for (i = 1; i <= total; i++)
	{
	  mu_message_t msg;
	  size_t msgsize;
	  status = mu_mailbox_get_message (mbox, i, &msg);
	  if (status)
	    return status;
	  status = mu_message_size (msg, &msgsize);
	  if (status)
	    return status;
	  size += msgsize;
	}
      *psize = size;
    }
  return status;
}

int
mu_mailbox_uidvalidity (mu_mailbox_t mbox, unsigned long *pvalid)
{
  _MBOX_CHECK_Q (mbox, _uidvalidity);
  return mbox->_uidvalidity (mbox, pvalid);
}

int
mu_mailbox_uidnext (mu_mailbox_t mbox, size_t *puidnext)
{
  _MBOX_CHECK_Q (mbox, _uidnext);
  return mbox->_uidnext (mbox, puidnext);
}

/* locking */
int
mu_mailbox_set_locker (mu_mailbox_t mbox, mu_locker_t locker)
{
  if (mbox == NULL)
    return EINVAL;
  if (mbox->locker)
    mu_locker_destroy (&mbox->locker);
  mbox->locker = locker;
  return 0;
}

int
mu_mailbox_get_locker (mu_mailbox_t mbox, mu_locker_t *plocker)
{
  if (mbox == NULL)
    return EINVAL;
  if (plocker == NULL)
    return MU_ERR_OUT_PTR_NULL;
  *plocker = mbox->locker;
  return 0;
}

int
mu_mailbox_get_flags (mu_mailbox_t mbox, int *flags)
{
  if (mbox == NULL)
    return EINVAL;
  if (!flags)
    return MU_ERR_OUT_PTR_NULL;
  *flags = mbox->flags & ~_MU_MAILBOX_MASK;
  return 0;
}

int
mu_mailbox_set_stream (mu_mailbox_t mbox, mu_stream_t stream)
{
  if (mbox == NULL)
    return EINVAL;
  if (mbox->flags & MU_STREAM_QACCESS)
    return MU_ERR_BADOP;
  if (mbox->stream)
    mu_stream_destroy (&mbox->stream);
  mbox->stream = stream;
  return 0;
}

int
mu_mailbox_get_observable (mu_mailbox_t mbox, mu_observable_t *pobservable)
{
  if (mbox == NULL)
    return EINVAL;
  if (pobservable == NULL)
    return MU_ERR_OUT_PTR_NULL;

  if (mbox->observable == NULL)
    {
      int status = mu_observable_create (&mbox->observable, mbox);
      if (status != 0)
	return status;
    }
  *pobservable = mbox->observable;
  return 0;
}

int
mu_mailbox_set_property (mu_mailbox_t mbox, mu_property_t property)
{
  if (mbox == NULL)
    return EINVAL;
  if (mbox->property)
    mu_property_unref (mbox->property);
  mbox->property = property;
  mu_property_ref (property);
  return 0;
}

int
mu_mailbox_get_property (mu_mailbox_t mbox, mu_property_t *pproperty)
{
  if (mbox == NULL)
    return EINVAL;
  if (pproperty == NULL)
    return MU_ERR_OUT_PTR_NULL;
  
  if (mbox->property == NULL)
    {
      int status;

      if (mbox->_get_property)
	status = mbox->_get_property (mbox, &mbox->property);
      else
	status = mu_property_create_init (&mbox->property,
					  mu_assoc_property_init, NULL);
      if (status != 0)
	return status;
    }
  *pproperty = mbox->property;
  return 0;
}

int
mu_mailbox_get_url (mu_mailbox_t mbox, mu_url_t *purl)
{
  if (mbox == NULL)
    return EINVAL;
  if (purl == NULL)
    return MU_ERR_OUT_PTR_NULL;
  *purl = mbox->url;
  return 0;
}

int
mu_mailbox_get_folder (mu_mailbox_t mbox, mu_folder_t *pfolder)
{
  if (mbox == NULL)
    return EINVAL;
  if (pfolder == NULL)
    return MU_ERR_OUT_PTR_NULL;
  *pfolder = mbox->folder;
  return 0;
}

int
mu_mailbox_set_folder (mu_mailbox_t mbox, mu_folder_t folder)
{
  if (mbox == NULL)
    return EINVAL;
  mbox->folder = folder;
  return 0;
}

int
mu_mailbox_lock (mu_mailbox_t mbox)
{
  mu_locker_t lock = NULL;
  mu_mailbox_get_locker (mbox, &lock);
  return mu_locker_lock (lock);
}

int
mu_mailbox_unlock (mu_mailbox_t mbox)
{
  mu_locker_t lock = NULL;
  mu_mailbox_get_locker (mbox, &lock);
  return mu_locker_unlock (lock);
}

int
mu_mailbox_get_uidls (mu_mailbox_t mbox, mu_list_t *plist)
{
  mu_list_t list;
  int status;

  if (mbox == NULL)
    return EINVAL;
  if (plist == NULL)
    return MU_ERR_OUT_PTR_NULL;
  status = mu_list_create (&list);
  if (status)
    return status;
  mu_list_set_destroy_item (list, mu_list_free_item);
  if (mbox->_get_uidls)
    status = mbox->_get_uidls (mbox, list);
  else
    {
      size_t i, total;

      status = mu_mailbox_messages_count (mbox, &total);
      if (status)
	return status;
      for (i = 1; i <= total; i++)
	{
	  mu_message_t msg = NULL;
	  char buf[MU_UIDL_BUFFER_SIZE];
	  size_t n;
	  struct mu_uidl *uidl;
	  
	  status = mu_mailbox_get_message (mbox, i, &msg);
	  if (status)
	    break;
	  status = mu_message_get_uidl (msg, buf, sizeof (buf), &n);
	  if (status)
	    break;
	  uidl = malloc (sizeof (uidl[0]));
	  if (!uidl)
	    {
	      status = ENOMEM;
	      break;
	    }
	  uidl->msgno = i;
	  strncpy (uidl->uidl, buf, MU_UIDL_BUFFER_SIZE);
	  status = mu_list_append (list, uidl);
	  if (status)
	    {
	      free (uidl);
	      break;
	    }
	}
    }
  *plist = list;
  return status;
}


/* Auxiliary function. Performs binary search for a message with the
   given UID number */
static int
_uid_bsearch (mu_mailbox_t mbox, size_t start, size_t stop, size_t uid,
	      size_t *msgno)
{
  mu_message_t mid_msg = NULL;
  size_t num = 0, middle;
  int rc;
  
  middle = (start + stop) / 2;
  rc = mu_mailbox_get_message (mbox, middle, &mid_msg);
  if (rc)
    return rc;
  rc = mu_message_get_uid (mid_msg, &num);
  if (rc)
    return rc;

  if (num == uid)
    {
      *msgno = middle;
      return 0;
    }
      
  if (start >= stop)
    return MU_ERR_NOENT;

  if (num > uid)
    return _uid_bsearch (mbox, start, middle - 1, uid, msgno);
  else /*if (num < seqno)*/
    return _uid_bsearch (mbox, middle + 1, stop, uid, msgno);
}

static int
_search_message_uid (mu_mailbox_t mbox, size_t uid, size_t *result)
{
  int rc;
  size_t num, count;
  mu_message_t msg;

  rc = mu_mailbox_get_message (mbox, 1, &msg);
  if (rc)
    return rc;
  rc = mu_message_get_uid (msg, &num);
  if (rc)
    return rc;
  if (uid < num)
    return MU_ERR_NOENT;
  else if (uid == num)
    {
      *result = 1;
      return 0;
    }

  rc = mu_mailbox_messages_count (mbox, &count);
  if (rc)
    return rc;
  rc = mu_mailbox_get_message (mbox, count, &msg);
  if (rc)
    return rc;
  rc = mu_message_get_uid (msg, &num);
  if (rc)
    return rc;

  if (uid > num)
    return MU_ERR_NOENT;
  else if (uid == num)
    {
      *result = count;
      return 0;
    }
  return _uid_bsearch (mbox, 1, count, uid, result);
}

/* Translate message UIDs to message numbers and vice versa. */
int
mu_mailbox_translate (mu_mailbox_t mbox, int cmd, size_t from, size_t *to)
{
  int rc = ENOSYS;
  mu_message_t msg;
  
  if (mbox == NULL)
    return EINVAL;
  if (to == NULL)
    return MU_ERR_OUT_PTR_NULL;
  if (mbox->flags & MU_STREAM_QACCESS)
    return MU_ERR_BADOP;

  if (mbox->_translate)
    rc = mbox->_translate (mbox, cmd, from, to);
  if (rc == ENOSYS)
    {
      switch (cmd)
	{
	case MU_MAILBOX_UID_TO_MSGNO:
	  rc = _search_message_uid (mbox, from, to);
	  break;
	  
	case MU_MAILBOX_MSGNO_TO_UID:
	  rc = mu_mailbox_get_message (mbox, from, &msg);
	  if (rc)
	    return rc;
	  rc = mu_message_get_uid (msg, to);
	  break;
	  
	default:
	  break;
	}
    }
  return rc;
}

