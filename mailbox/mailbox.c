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
 * Then we call the appropriate mailbox_*type*_create() function.
 */
int
mailbox_create (mailbox_t *pmbox, const char *name, int id)
{
  int status = EINVAL;
  struct mailbox_registrar *mreg;
  url_t url = NULL;

  url_create (&url, name);

  /* 1st guest: if an ID is specify, shortcut */
  if (id)
    {
      status = registrar_get (id, NULL, &mreg);
      if (status == 0)
	status = mreg->_create (pmbox, name);
    }
  /* 2nd fallback: Use the URL */
  else if (url != NULL)
    {
      url_get_id (url, &id);
      status = registrar_get (id, NULL, &mreg);
      if (status == 0)
	status = mreg->_create (pmbox, name);
    }

  /* set the URL */
  if (status == 0)
    (*pmbox)->url = url;
  else
    url_destroy (&url);
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
    return EINVAL;
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
mailbox_num_deleted (mailbox_t mbox, size_t *num)
{
  if (mbox == NULL || mbox->_num_deleted == NULL)
    return EINVAL;
  return mbox->_num_deleted (mbox, num);
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
  if (pauth)
    *pauth = mbox->auth;
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
mailbox_register (mailbox_t mbox, size_t type,
		  int (*action) (size_t type, void *arg),
		  void *arg)
{
  size_t i;
  event_t event;

  /* FIXME: I should check for invalid types */
  if (mbox == NULL || action == NULL)
    return EINVAL;

  /* find a free spot */
  for (i = 0; i < mbox->event_num; i++)
    {
      event = &(mbox->event[i]);
      if (event->_action == NULL)
	{
	  event->_action = action;
	  event->type = type;
	  event->arg = arg;
	  return 0;
	}
    }

  /* a new one */
  event = realloc (mbox->event, (mbox->event_num + 1) * sizeof (*event));
  if (event == NULL)
    return ENOMEM;

  mbox->event = event;
  event[mbox->event_num]._action = action;
  event[mbox->event_num].type = type;
  event[mbox->event_num].arg = arg;
  mbox->event_num++;
  return 0;
}

int
mailbox_deregister (mailbox_t mbox, void *action)
{
  size_t i;
  event_t event;

  for (i = 0; i < mbox->event_num; i++)
    {
      event = &(mbox->event[i]);
      if ((int)event->_action == (int)action)
	{
	  event->type = 0;
	  event->_action = NULL;
	  event->arg = NULL;
	  return 0;
	}
    }
  return ENOENT;
}

int
mailbox_notification (mailbox_t mbox, size_t type)
{
  size_t i;
  event_t event;
  int status = 0;
  for (i = 0; i < mbox->event_num; i++)
    {
      event = &(mbox->event[i]);
      if ((event->_action) &&  (event->type & type))
	status |= event->_action (type, event->arg);
    }
  return status;
}

int
mailbox_set_debug_level (mailbox_t mbox, size_t level)
{
  if (mbox == NULL)
    return EINVAL;
  mbox->debug_level = level;
  return 0;
}

int
mailbox_get_debug_level (mailbox_t mbox, size_t *plevel)
{
  if (mbox == NULL || plevel == NULL)
    return EINVAL;
  *plevel = mbox->debug_level;
  return 0;
}

int
mailbox_set_debug_print (mailbox_t mbox, int (*debug_print)
			 (const char *, void *arg), void *arg)
{
  if (mbox == NULL)
    return EINVAL;
  mbox->debug_print = debug_print;
  mbox->debug_arg = arg;
  return 0;
}
