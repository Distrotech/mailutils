/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999 Free Software Foundation, Inc.

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
mailbox_close (mailbox_t mbox, int flag)
{
  return mbox->_close (mbox, flag);
}


/* type */
int
mailbox_get_type (mailbox_t mbox, int *type, char *desc, int offset, int len)
{
  return mbox->_get_type (mbox, type, desc, offset, len);
}

int
mailbox_get_mtype (mailbox_t mbox, int *type, char **desc, int *len)
{
  return mbox->_get_mtype (mbox, type, desc, offset, len);
}


/* passwd */
int
mailbox_get_passwd (mailbox_t mbox, char *passwd, int offset, int len)
{
  return mbox->_get_passwd (mbox, passwd, offset, len);
}

int
mailbox_get_mpasswd (mailbox_t mbox, char **passwd, int *len)
{
  return mbox->_get_mpasswd (mbox, passwd, len);
}

int
mailbox_set_passwd  (mailbox_t mbox, const char *passwd, int offset, int len)
{
  return mbox->_set_passwd (mbox, passwd, offset, len);
}


/* deleting */
int
mailbox_delete (mailbox_t mbox, int id)
{
  return mbox->_delete (mbox, id);
}

int
mailbox_undelete (mailbox_t mbox, int id)
{
  return mbox->_undelete (mbox, id);
}

int
mailbox_expunge (mailbox_t mbox)
{
  return mbox->_expunge (mbox);
}

int
mailbox_is_deleted (mailbox_t mbox, int)
{
  return mbox->_is_deleted (mbox, int);
}


/* appending */
int
mailbox_new_msg (mailbox_t mbox, int * id)
{
  return mbox->_new_msg (mbox, id);
}

int
mailbox_set_header (mailbox_t mbox, int id, const char *h,
		    int offset, int n, int replace)
{
  return mbox->_set_header (mbox, id, h, offset, n, replace);
}

int
mailbox_set_body (mailbox_t mbox, int id, const char *b,
		  int offset, int n, int replace)
{
  return mbox->_set_body (mbox, id, b, offset, n, replace);
}

int
mailbox_append (mailbox_t mbox, int id)
{
  return mbox->_append (mbox, id);
}

int
mailbox_destroy_msg (mailbox_t mbox, int id)
{
  return mbox->_destroy_msg (mbox, id);
}


/* reading */
int
mailbox_get_body (mailbox_t mbox, int id, char *b, int offset, int n)
{
  return mbox->_get_body (mbox, id, b, offset, n);
}

int
mailbox_get_mbody (mailbox_t mbox, int id, char **b, int *n)
{
  return mbox->_get_body (mbox, id, b, n);
}

int
mailbox_get_header (mailbox_t mbox, int id, char *h, int offset, int n)
{
  return mbox->_get_header (mbox, id, h, offset, n);
}

int
mailbox_get_mheader (mailbox_t mbox, int id, char **h, int *n)
{
  return mbox->_get_header (mbox, id, h, n);
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
mailbox_scan (mailbox_t mbox, int *msgs)
{
  return mbox->_scan (mbox, msgs);
}

int
mailbox_is_updated (mailbox_t mbox)
{
  return mbox->_is_updated (mbox);
}

int
mailbox_get_timeout (mailbox_t mbox, int *timeout)
{
  return mbox->_get_timeout (mbox, timeout);
}

int
mailbox_set_timeout (mailbox_t mbox, int timeout)
{
  return mbox->_set_timeout (mbox, timeout);
}

int
mailbox_get_refresh (mailbox_t mbox, int *refresh)
{
  return mbox->_get_refresh (mbox, refresh);
}

int
mailbox_set_refresh (mailbox_t mbox, int refresh)
{
  return mbox->_set_refresh (mbox, refresh);
}

int
mailbox_set_notification (mailbox_t mbox, int (*notif) __P ((mailbox_t mbox)))
{
  return mbox->_set_notification (mbox, notif);
}
