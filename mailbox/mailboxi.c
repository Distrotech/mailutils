/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU Library General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#include <mailbox.h>

int
mailbox_open (mailbox_t mbox, int flag)
{
  return mbox->_open (mbox, flag);
}

int
mailbox_close (mailbox_t mbox)
{
  return mbox->_close (mbox);
}


/* passwd */
int
mailbox_get_passwd (mailbox_t mbox, char *passwd, size_t len, size_t *n)
{
  return mbox->_get_passwd (mbox, passwd, len, n);
}

int
mailbox_get_mpasswd (mailbox_t mbox, char **passwd, size_t *n)
{
  return mbox->_get_mpasswd (mbox, passwd, n);
}

int
mailbox_set_passwd  (mailbox_t mbox, const char *passwd,
		     size_t len)
{
  return mbox->_set_passwd (mbox, passwd, len);
}


/* deleting */
int
mailbox_delete (mailbox_t mbox, size_t msgno)
{
  return mbox->_delete (mbox, msgno);
}

int
mailbox_undelete (mailbox_t mbox, size_t msgno)
{
  return mbox->_undelete (mbox, msgno);
}

int
mailbox_expunge (mailbox_t mbox)
{
  return mbox->_expunge (mbox);
}

int
mailbox_is_deleted (mailbox_t mbox, size_t msgno)
{
  return mbox->_is_deleted (mbox, msgno);
}


/* appending */
int
mailbox_new_msg (mailbox_t mbox, size_t *msgno)
{
  return mbox->_new_msg (mbox, msgno);
}

int
mailbox_set_header (mailbox_t mbox, size_t msgno, const char *h,
		    size_t len, int replace)
{
  return mbox->_set_header (mbox, msgno, h, len, replace);
}

int
mailbox_set_body (mailbox_t mbox, size_t msgno, const char *b,
		  size_t len, int replace)
{
  return mbox->_set_body (mbox, msgno, b, len, replace);
}

int
mailbox_append (mailbox_t mbox, size_t msgno)
{
  return mbox->_append (mbox, msgno);
}

int
mailbox_destroy_msg (mailbox_t mbox, size_t msgno)
{
  return mbox->_destroy_msg (mbox, msgno);
}


/* reading */
int
mailbox_get_body (mailbox_t mbox, size_t msgno, off_t off, char *b,
		  size_t len, size_t *n)
{
  return mbox->_get_body (mbox, msgno, off, b, len, n);
}

int
mailbox_get_mbody (mailbox_t mbox, size_t msgno, off_t off,
		   char **b, size_t *n)
{
  return mbox->_get_mbody (mbox, msgno, off, b, n);
}

int
mailbox_get_header (mailbox_t mbox, size_t msgno, off_t off, char *h,
		    size_t len, size_t *n)
{
  return mbox->_get_header (mbox, msgno, off, h, len, n);
}

int
mailbox_get_mheader (mailbox_t mbox, size_t msgno, off_t off,
		     char **h, size_t *n)
{
  return mbox->_get_mheader (mbox, msgno, off, h, n);
}


/* locking */
int
mailbox_lock (mailbox_t mbox, int flag)
{
  return mbox->_lock (mbox, flag);
}

int
mailbox_unlock (mailbox_t mbox)
{
  return mbox->_unlock (mbox);
}


/* misc */
int
mailbox_scan (mailbox_t mbox, size_t *msgs)
{
  return mbox->_scan (mbox, msgs);
}

int
mailbox_is_updated (mailbox_t mbox)
{
  return mbox->_is_updated (mbox);
}

int
mailbox_get_timeout (mailbox_t mbox, size_t *timeout)
{
  return mbox->_get_timeout (mbox, timeout);
}

int
mailbox_set_timeout (mailbox_t mbox, size_t timeout)
{
  return mbox->_set_timeout (mbox, timeout);
}

int
mailbox_get_refresh (mailbox_t mbox, size_t *refresh)
{
  return mbox->_get_refresh (mbox, refresh);
}

int
mailbox_set_refresh (mailbox_t mbox, size_t refresh)
{
  return mbox->_set_refresh (mbox, refresh);
}

int
mailbox_set_notification (mailbox_t mbox,
			  int (*notif) __P ((mailbox_t, void *)))
{
  return mbox->_set_notification (mbox, notif);
}
