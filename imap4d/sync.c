/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2001 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#include "imap4d.h"

/*

 */
struct _uid_table
{
  size_t uid;
  size_t msgno;
  int notify;
  attribute_t attr;
};

static struct _uid_table *uid_table;
static size_t uid_table_count;

static void
add_flag (char **pbuf, const char *f)
{
  char *abuf = *pbuf;
  abuf = realloc (abuf, strlen (abuf) + strlen (f) + 2);
  if (abuf == NULL)
    util_quit (ERR_NO_MEM);
  if (*abuf)
    strcat (abuf, " ");
  strcat (abuf, "\\Seen");
  *pbuf = abuf;
}

static void
notify_flag (size_t msgno, attribute_t oattr)
{
  message_t msg = NULL;
  attribute_t nattr = NULL;
  int status ;
  mailbox_get_message (mbox, msgno, &msg);
  message_get_attribute (msg, &nattr);
  status = attribute_is_equal (oattr, nattr);
  //if (!attribute_is_equal (oattr, nattr))
  if (status == 0)
    {
      char *abuf = malloc (1);;
      if (!abuf)
	util_quit (ERR_NO_MEM);
      *abuf = '\0';
      if (attribute_is_seen (nattr) && attribute_is_read (nattr))
	if (!attribute_is_seen (oattr) && !attribute_is_read (oattr))
	  {
	    attribute_set_seen (oattr);
	    attribute_set_read (oattr);
	    add_flag (&abuf, "\\Seen");
	  }
      if (attribute_is_answered (nattr))
	if (!attribute_is_answered (oattr))
	  {
	    attribute_set_answered (oattr);
	    add_flag (&abuf, "\\Answered");
	  }
      if (attribute_is_flagged (nattr))
	if (!attribute_is_flagged (oattr))
	  {
	    attribute_set_flagged (oattr);
	    add_flag (&abuf, "\\Flagged");
	  }
      if (attribute_is_deleted (nattr))
	if (!attribute_is_deleted (oattr))
	  {
	    attribute_set_deleted (oattr);
	    add_flag (&abuf, "\\Deleted");
	  }
      if (attribute_is_draft (nattr))
	if (!attribute_is_draft (oattr))
	  {
	    attribute_set_draft (oattr);
	    add_flag (&abuf, "\\Draft");
	  }
      if (attribute_is_recent (nattr))
	if (!attribute_is_recent (oattr))
	  {
	    attribute_set_recent (oattr);
	    add_flag (&abuf, "\\Recent");
	  }
      if (*abuf)
	util_out (RESP_NONE, "%d FETCH FLAGS (%s)", msgno, abuf);
      free (abuf);
    }
}

static void
notify_deleted (void)
{
  if (uid_table)
    {
      size_t i;
      for (i = 0; i < uid_table_count; i++)
	{
	  if (!(uid_table[i].notify))
	    {
	      util_out (RESP_NONE, "%d EXPUNGE", uid_table[i].msgno);
	      uid_table[i].notify = 1;
	    }
	}
    }
}


static int
notify_uid (size_t uid)
{
  if (uid_table)
    {
      size_t i;
      for (i = 0; i < uid_table_count; i++)
	{
	  if (uid_table[i].uid == uid)
	    {
	      notify_flag (uid_table[i].msgno, uid_table[i].attr);
	      uid_table[i].notify = 1;
	      return 1;
	    }
	}
    }
  return 0;
}

static void
notify (void)
{
  if (uid_table)
    {
      size_t total = 0;
      size_t i;
      size_t recent = 0;
      mailbox_messages_count (mbox, &total);
      for (i = 1; i <= total; i++)
	{
	  message_t msg = NULL;
	  size_t uid = 0;
	  mailbox_get_message (mbox, i, &msg);
	  message_get_uid (msg, &uid);
	  if (!notify_uid (uid))
	    recent++;
	}
      util_out (RESP_NONE, "%d EXISTS", total);
      if (recent)
	util_out (RESP_NONE, "%d RECENT", recent);
      notify_deleted ();
    }
}

static void
free_uids (void)
{
  if (uid_table)
    {
      size_t i;
      for (i = 0; i < uid_table_count; i++)
	attribute_destroy (&(uid_table[i].attr), NULL);
      free (uid_table);
      uid_table = NULL;
      uid_table_count = 0;
    }
}

static void
reset_uids (void)
{
  size_t total = 0;
  size_t i;

  notify ();
  free_uids ();

  mailbox_messages_count (mbox, &total);
  for (i = 1; i <= total; i++)
    {
      message_t msg = NULL;
      attribute_t attr = NULL;
      size_t uid = 0;
      uid_table = realloc (uid_table, sizeof (*uid_table) *
			   (uid_table_count + 1));
      if (!uid_table)
	util_quit (ERR_NO_MEM);
      mailbox_get_message (mbox, i, &msg);
      message_get_attribute (msg, &attr);
      message_get_uid (msg, &uid);
      uid_table[uid_table_count].uid = uid;
      uid_table[uid_table_count].msgno = i;
      uid_table[uid_table_count].notify = 0;
      attribute_create (&(uid_table[uid_table_count].attr), NULL);
      attribute_copy (uid_table[uid_table_count].attr, attr);
      uid_table_count++;
    }
}

size_t
uid_to_msgno (size_t uid)
{
  size_t i;
  for (i = 0; i < uid_table_count; i++)
    if (uid_table[i].uid == uid)
      return uid_table[i].msgno;
  return 0;
}

int
imap4d_sync_flags (size_t msgno)
{
  size_t i;
  for (i = 0; i < uid_table_count; i++)
    if (uid_table[i].msgno == msgno)
      {
	message_t msg = NULL;
	attribute_t attr = NULL;
	mailbox_get_message (mbox, msgno, &msg);
	message_get_attribute (msg, &attr);
	attribute_copy (uid_table[i].attr, attr);
	break;
      }
  return 0;
}

int
imap4d_sync (void)
{
  /* if mbox --> NULL, it means to free all the ressources.
     it may be because of close or before select/examine a new mailbox.  */
  if (mbox == NULL)
    free_uids ();
  else if (uid_table == NULL || !mailbox_is_updated (mbox))
    reset_uids ();
  return 0;
}
