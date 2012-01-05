/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001, 2005, 2007, 2009-2012 Free Software
   Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>. */

#include "imap4d.h"
#include <mailutils/observer.h>

/*

 */
static int *attr_table;
static size_t attr_table_count;
static size_t attr_table_max;
static int attr_table_valid;

static void
realloc_attributes (size_t total)
{
  if (total > attr_table_max)
    {
      attr_table = realloc (attr_table, sizeof (*attr_table) * total);
      if (!attr_table)
	imap4d_bye (ERR_NO_MEM);
      attr_table_max = total;
    }
  attr_table_count = total;
}

void
imap4d_sync_invalidate ()
{
  attr_table_valid = 0;
  attr_table_count = 0;
}

static void
reread_attributes (void)
{
  size_t total = 0;
  size_t i;

  mu_mailbox_messages_count (mbox, &total);
  realloc_attributes (total);
  for (i = 1; i <= total; i++)
    {
      mu_message_t msg = NULL;
      mu_attribute_t attr = NULL;

      mu_mailbox_get_message (mbox, i, &msg);
      mu_message_get_attribute (msg, &attr);
      mu_attribute_get_flags (attr, &attr_table[i-1]);
    }
  attr_table_valid = 1;
}

static void
notify (void)
{
  size_t total = 0;
  size_t recent = 0;
  
  mu_mailbox_messages_count (mbox, &total);
  mu_mailbox_messages_recent (mbox, &recent);

  if (!attr_table_valid)
    {
      reread_attributes ();
    }
  else 
    {
      size_t old_total = attr_table_count;
      size_t i;

      realloc_attributes (total);
      for (i = 1; i <= total; i++)
	{
	  mu_message_t msg = NULL;
	  mu_attribute_t nattr = NULL;
	  int nflags;
	  
	  mu_mailbox_get_message (mbox, i, &msg);
	  mu_message_get_attribute (msg, &nattr);
	  mu_attribute_get_flags (nattr, &nflags);

	  if (i <= old_total)
	    {
	      if (nflags != attr_table[i-1])
		{
		  io_sendf ("* %lu FETCH FLAGS (",  (unsigned long) i);
		  mu_imap_format_flags (iostream, nflags, 1);
		  io_sendf (")\n");
		  attr_table[i-1] = nflags;
		}
	    }
	  else
	    attr_table[i-1] = nflags;
	}
    }
  
  io_untagged_response (RESP_NONE, "%lu EXISTS", (unsigned long) total);
  io_untagged_response (RESP_NONE, "%lu RECENT", (unsigned long) recent);
}

size_t
uid_to_msgno (size_t uid)
{
  size_t msgno;
  mu_mailbox_translate (mbox, MU_MAILBOX_UID_TO_MSGNO, uid, &msgno);
  return msgno;
}

int
imap4d_sync_flags (size_t msgno)
{
  if (attr_table)
    {
      mu_message_t msg = NULL;
      mu_attribute_t attr = NULL;
      mu_mailbox_get_message (mbox, msgno, &msg);
      mu_message_get_attribute (msg, &attr);
      mu_attribute_get_flags (attr, &attr_table[msgno-1]);
    }
  return 0;
}

int silent_expunge;
/* When true, non-tagged EXPUNGE responses are suppressed. */

static int mailbox_corrupt;
/* When true, mailbox has been altered by another party. */

static int
action (mu_observer_t observer, size_t type, void *data, void *action_data)
{
  switch (type)
    {
    case MU_EVT_MAILBOX_CORRUPT:
      mailbox_corrupt = 1;
      break;

    case MU_EVT_MAILBOX_DESTROY:
      mailbox_corrupt = 0;
      break;

    case MU_EVT_MAILBOX_MESSAGE_EXPUNGE:
      /* The EXPUNGE response reports that the specified message sequence
	 number has been permanently removed from the mailbox.  The message
	 sequence number for each successive message in the mailbox is
	 immediately decremented by 1, and this decrement is reflected in
	 message sequence numbers in subsequent responses (including other
	 untagged EXPUNGE responses). */
      if (!silent_expunge)
	{
	  size_t *exp = data;
	  io_untagged_response (RESP_NONE, "%lu EXPUNGED",
				(unsigned long) (exp[0] - exp[1]));
	}
    }
  return 0;
}

void
imap4d_set_observer (mu_mailbox_t mbox)
{
  mu_observer_t observer;
  mu_observable_t observable;
      
  mu_observer_create (&observer, mbox);
  mu_observer_set_action (observer, action, mbox);
  mu_mailbox_get_observable (mbox, &observable);
  mu_observable_attach (observable,
			MU_EVT_MAILBOX_CORRUPT|
			MU_EVT_MAILBOX_DESTROY|
			MU_EVT_MAILBOX_MESSAGE_EXPUNGE,
			observer);
  mailbox_corrupt = 0;
}

int
imap4d_sync (void)
{
  manlock_touchlock (mbox);
  /* If mbox --> NULL, it means to free all the resources.
     It may be because of close or before select/examine a new mailbox.
     If it was a close we do not send any notification.  */
  if (mbox == NULL)
    imap4d_sync_invalidate ();
  else if (!attr_table_valid || !mu_mailbox_is_updated (mbox))
    {
      if (mailbox_corrupt)
	{
	  /* Some messages have been deleted from the mailbox by some other
	     party */
	  int status = mu_mailbox_close (mbox);
	  if (status)
	    imap4d_bye (ERR_MAILBOX_CORRUPTED);
	  status = mu_mailbox_open (mbox, MU_STREAM_RDWR);
	  if (status)
	    imap4d_bye (ERR_MAILBOX_CORRUPTED);
	  imap4d_set_observer (mbox);
	  imap4d_sync_invalidate ();
	  mailbox_corrupt = 0;
	  io_untagged_response (RESP_NONE,
		             "OK [ALERT] Mailbox modified by another program");
	}
      notify ();
    }
  else
    {
      size_t count = 0;
      mu_mailbox_messages_count (mbox, &count);
      if (count != attr_table_count)
	notify ();
    }
  return 0;
}
