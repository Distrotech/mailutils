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

#include <mailbox0.h>
#include <message0.h>
#include <registrar.h>
#include <locker.h>

#include <stdlib.h>
#include <string.h>
#include <errno.h>

/*
 * Point of entry.
 * Simple, first check if they ask for something specific; with the ID.
 * Then try to discover the type of mailbox with the url(name).
 * Then we call the appropriate mailbox_*type*_init() function.
 */
int
mailbox_init (mailbox_t *pmbox, const char *name, int id)
{
  int status = EINVAL;
  struct mailbox_registrar *mreg;
  url_t url = NULL;

  url_init (&url, name);

  /* 1st guest: if an ID is specify, shortcut */
  if (id)
    {
      status = registrar_get (id, NULL, &mreg);
      if (status == 0)
	status = mreg->_init (pmbox, name);
    }
  /* 2nd fallback: Use the URL */
  else if (url != NULL)
    {
      url_get_id (url, &id);
      status = registrar_get (id, NULL, &mreg);
      if (status == 0)
	status = mreg->_init (pmbox, name);
    }

  /* if things went ok set mreg for mailbox_destroy and the URL */
  if (status == 0)
    (*pmbox)->url = url;
  return status;
}

void
mailbox_destroy (mailbox_t *pmbox)
{
  if (pmbox && *pmbox)
    (*pmbox)->_destroy (pmbox);
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
mailbox_is_deleted (mailbox_t mbox, size_t msgno)
{
  if (mbox == NULL || mbox->_is_deleted == NULL)
    return 0;
  return mbox->_is_deleted (mbox, msgno);
}

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
  int status;
  if (mbox == NULL || pmsg == NULL)
    return EINVAL;
  if (mailbox_is_deleted (mbox, msgno))
    return EINVAL;
  status = message_init (pmsg);
  if (status != 0)
    return status;
  (*pmsg)->mailbox = mbox;
  (*pmsg)->num = msgno;
  return 0;
}

int
mailbox_messages_count (mailbox_t mbox, size_t *num)
{
  if (mbox == NULL || mbox->_messages_count == NULL)
    return ENOSYS;
  return mbox->_messages_count (mbox, num);
}

int
mailbox_delete (mailbox_t mbox, size_t msgno)
{
  if (mbox == NULL || mbox->_delete == NULL)
    return ENOSYS;
  return mbox->_delete (mbox, msgno);
}

int
mailbox_undelete (mailbox_t mbox, size_t msgno)
{
  if (mbox == NULL || mbox->_undelete == NULL)
    return ENOSYS;
  return mbox->_undelete (mbox, msgno);
}

int
mailbox_expunge (mailbox_t mbox)
{
  if (mbox == NULL || mbox->_expunge == NULL)
    return ENOSYS;
  return mbox->_expunge (mbox);
}

int
mailbox_num_deleted (mailbox_t mbox, size_t *num)
{
  if (mbox == NULL || mbox->_num_deleted == NULL)
    return EINVAL;
  return mbox->_num_deleted (mbox, num);
}

/* hook to allow some sort of "progress bar" .. I don't like it
 * but can not find any other way yet
 */
int
mailbox_progress (mailbox_t mbox, int (*progress) (int, void *arg), void *arg)
{
  if (mbox == NULL)
    return EINVAL;
  mbox->_progress = progress;
  mbox->progress_arg = arg;
  return 0;
}

int
mailbox_is_updated (mailbox_t mbox)
{
  if (mbox == NULL || mbox->_is_updated == NULL)
    return 0;
  return mbox->_is_updated (mbox);
}

ssize_t
mailbox_get_size (mailbox_t mbox, size_t msgno, size_t *h, size_t *b)
{
  if (mbox == NULL || mbox->_get_size == NULL)
    return EINVAL;
  return mbox->_get_size (mbox, msgno, h, b);
}

ssize_t
mailbox_get_header (mailbox_t mbox, size_t msgno, char *h,
		    size_t len, off_t off, int *err)
{
  if (mbox == NULL || mbox->_get_header == NULL)
    return ENOSYS;
  return mbox->_get_header (mbox, msgno, h, len, off, err);
}

ssize_t
mailbox_get_body (mailbox_t mbox, size_t msgno, char *b,
		  size_t len, off_t off, int *err)
{
  if (mbox == NULL || mbox->_get_body == NULL)
    return ENOSYS;
  return mbox->_get_body (mbox, msgno, b, len, off, err);
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
mailbox_get_locker (mailbox_t mbox, locker_t *locker)
{
  if (mbox == NULL || locker == NULL)
    return EINVAL;
  *locker = mbox->locker;
  return 0;
}

int
mailbox_set_auth (mailbox_t mbox, auth_t auth)
{
  if (mbox == NULL)
    return EINVAL;
  mbox->auth = auth;
  return 0;
}

int
mailbox_get_auth (mailbox_t mbox, auth_t *pauth)
{
  if (mbox == NULL || pauth == NULL)
    return EINVAL;
  *pauth = mbox->auth;
  return 0;
}

int
mailbox_get_attribute (mailbox_t mbox, size_t msgno, attribute_t *pattr)
{
  if (mbox == NULL || mbox->_get_attribute == NULL)
    return ENOSYS;
  return mbox->_get_attribute (mbox, msgno, pattr);
}

int
mailbox_set_attribute (mailbox_t mbox, size_t msgno, attribute_t attr)
{
  if (mbox == NULL || mbox->_set_attribute == NULL)
    return ENOSYS;
  return mbox->_set_attribute (mbox, msgno, attr);
}
