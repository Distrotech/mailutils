/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999 Free Software Foundation, Inc.

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

#include <mbx_mbox.h>
#include <mbx_pop.h>
#include <mbx_imap.h>

#include <stdlib.h>
#include <errno.h>

/* forward prototypes */
static int mbx_open        (mailbox_t, int flag);
static int mbx_close       (mailbox_t, int flag);

/* name */
static int mbx_get_name    (mailbox_t, int *id, char *name, int offset, int n);

/* passwd */
static int mbx_get_passwd  (mailbox_t, char *passwd, int offset, int len);
static int mbx_get_mpasswd (mailbox_t, char **passwd, int *len);
static int mbx_set_passwd  (mailbox_t, const char *passwd, int offset, int n);

/* deleting */
static int mbx_delete      (mailbox_t, int id);
static int mbx_undelete    (mailbox_t, int id);
static int mbx_expunge     (mailbox_t);
static int mbx_is_deleted  (mailbox_t, int id);

/* appending */
static int mbx_new_msg     (mailbox_t, int *id);
static int mbx_set_header  (mailbox_t, int id, const char *h,
			    int offset, int n, int replace);
static int mbx_set_body    (mailbox_t, int id, const char *b,
			    int offset, int n, int replace);
static int mbx_append      (mailbox_t, int id);
static int mbx_destroy_msg (mailbox_t, int id);

/* reading */
static int mbx_get_body    (mailbox_t, int id, char *b, int offset, int n);
static int mbx_get_mbody   (mailbox_t, int id, char **b, int *n);
static int mbx_get_header  (mailbox_t, int id, char *h, int offset, int n);
static int mbx_get_mheader (mailbox_t, int id, char **h, int *n);

/* locking */
static int mbx_lock        (mailbox_t, int flag);
static int mbx_unlock      (mailbox_t);

/* misc */
static int mbx_scan        (mailbox_t, int *msgs);
static int mbx_is_updated  (mailbox_t);
static int mbx_get_timeout (mailbox_t, int *timeout);
static int mbx_set_timeout (mailbox_t, int timeout);
static int mbx_get_refresh (mailbox_t, int *refresh);
static int mbx_set_refresh (mailbox_t, int refresh);
static int mbx_get_size    (mailbox_t, int id, size_t *size);
static int mbx_set_notification (mailbox_t,
				 int (*func) (mailbox_t, void *arg));

/* init all the functions to a default value */
static void mbx_check_struct (mailbox_t);

/*
  Builtin mailbox types.
  A circular list is use for the builtin.
  Proper locking is not done when accessing the list.
  FIXME: not thread-safe. */
static struct mailbox_builtin
{
  struct mailbox_type *mtype;
  int is_malloc;
  struct mailbox_builtin * next;
} mailbox_builtin [] = {
  { NULL, 0,                  &mailbox_builtin[1] }, /* sentinel, head list */
  { &_mailbox_mbox_type, 0,   &mailbox_builtin[2] },
  { &_mailbox_pop_type, 0,    &mailbox_builtin[3] },
  { &_mailbox_imap_type, 0,   &mailbox_builtin[0] },
};

int
malibox_add_type (struct mailbox_type *mtype)
{
  struct mailbox_builtin *current = malloc (sizeof (*current));
  if (current == NULL)
    return -1;
  if (mtype->utype)
    {
      if (url_add_type (mtype->utype) < 0)
	{
	  free (current);
	  return -1;
	}
      mtype->id = mtype->utype->id; /* same ID as the url_type */
    }
  else
    {
      mtype->id = (int)mtype; /* just need to be uniq */
    }
  current->mtype = mtype;
  current->is_malloc = 1;
  current->next = mailbox_builtin->next;
  mailbox_builtin->next = current;
  return 0;
}

int
mailbox_remove_type (struct mailbox_type *mtype)
{
  struct mailbox_builtin *current, *previous;
  for (previous = mailbox_builtin, current = mailbox_builtin->next;
       current != mailbox_builtin;
       previous = current, current = current->next)
    {
      if (current->mtype == mtype)
        {
          previous->next = current->next;
          if (current->is_malloc)
            free (current);
          return 0;;
        }
    }
  return -1;
}

int
mailbox_list_type (struct mailbox_type *list, int n)
{
  struct mailbox_builtin *current;
  int i;
  for (i = 0, current = mailbox_builtin->next; current != mailbox_builtin;
       current = current->next, i++)
    {
      if (list)
	if (i < n)
	  list[i] = *(current->mtype);
    }
  return i;
}

int
mailbox_list_mtype (struct mailbox_type **mlist, int *n)
{
  struct mailbox_type *mtype;
  int i;

  if ((i = mailbox_list_type (NULL, 0)) <= 0 || (mtype = malloc (i)) == NULL)
    {
      return -1;
    }
  *mlist = mtype;
  return *n = mailbox_list_type (mtype, i);
}

int
mailbox_get_type (struct mailbox_type **mtype, int id)
{
  struct mailbox_builtin *current;
  for (current = mailbox_builtin->next; current != mailbox_builtin;
       current = current->next)
    {
      if (current->mtype->id == id)
        {
	  *mtype = current->mtype;
          return 0;;
        }
    }
  return -1;
}


int
mailbox_init (mailbox_t *mbox, const char *name, int id)
{
  int status = -1; /* -1 means error */

  /* 1st run: if they know what they want, shortcut */
  if (id)
    {
      struct mailbox_type *mtype;
      status = mailbox_get_type (&mtype, id);
      if (status == 0)
	{
	  status = mtype->_init (mbox, name);
	  if (status == 0)
	    mbx_check_struct (*mbox);
	}
    }

  /* 2nd run: try heuristic URL discovery */
  if (status != 0)
    {
      url_t url;
      status = url_init (&url, name);
      if (status == 0)
	{
	  int id;
	  struct mailbox_type *mtype;
	  /* We've found a match get it */
	  if (url_get_id (url, &id) == 0
	      && (status = mailbox_get_type (&mtype, id)) == 0)
	    {
	      status = mtype->_init (mbox, name);
	      if (status == 0)
		mbx_check_struct (*mbox);
	    }
	  url_destroy (&url);
	}
    }

  /* 3nd run: nothing yet ?? try unixmbox/maildir directly as the last resort.
     this should take care of the case where the filename is use */
  if (status != 0 )
    {
      status = mailbox_mbox_init (mbox, name);
      if (status == 0)
	mbx_check_struct (*mbox);
    }
  return status;
}

void
mailbox_destroy (mailbox_t *mbox)
{
  struct mailbox_type *mtype = (*mbox)->mtype;
  return mtype->_destroy (mbox);
}

/* -------------- stub functions ------------------- */
static void
mbx_check_struct (mailbox_t mbox)
{
  if (mbox->_open == NULL)
    mbox->_open = mbx_open;

  if (mbox->_close == NULL)
    mbox->_close = mbx_close;

  if (mbox->_get_name == NULL)
    mbox->_get_name = mbx_get_name;

  if (mbox->_get_passwd == NULL)
    mbox->_get_passwd = mbx_get_passwd;

  if (mbox->_get_mpasswd == NULL)
    mbox->_get_mpasswd = mbx_get_mpasswd;

  if (mbox->_set_passwd  == NULL)
    mbox->_set_passwd = mbx_set_passwd;

  if (mbox->_delete == NULL)
    mbox->_delete = mbx_delete;

  if (mbox->_undelete == NULL)
    mbox->_undelete = mbx_undelete;

  if (mbox->_expunge == NULL)
    mbox->_expunge = mbx_expunge;

  if (mbox->_is_deleted == NULL)
    mbox->_is_deleted = mbx_is_deleted;

  if (mbox->_new_msg == NULL)
    mbox->_new_msg = mbx_new_msg;

  if (mbox->_set_header == NULL)
    mbox->_set_header = mbx_set_header;

  if (mbox->_set_body == NULL)
    mbox->_set_body = mbx_set_body;

  if (mbox->_append == NULL)
    mbox->_append = mbx_append;

  if (mbox->_destroy_msg == NULL)
    mbox->_destroy_msg = mbx_destroy_msg;

  if (mbox->_get_body == NULL)
    mbox->_get_body = mbx_get_body;

  if (mbox->_get_mbody == NULL)
    mbox->_get_mbody = mbx_get_mbody;

  if (mbox->_get_header == NULL)
    mbox->_get_header = mbx_get_header;

  if (mbox->_get_mheader == NULL)
    mbox->_get_mheader = mbx_get_mheader;

  if (mbox->_lock == NULL)
    mbox->_lock = mbx_lock;

  if (mbox->_unlock == NULL)
    mbox->_unlock = mbx_unlock;

  if (mbox->_scan == NULL)
    mbox->_scan = mbx_scan;

  if (mbox->_is_updated == NULL)
    mbox->_is_updated = mbx_is_updated;

  if (mbox->_get_timeout == NULL)
    mbox->_get_timeout = mbx_get_timeout;

  if (mbox->_set_timeout == NULL)
    mbox->_set_timeout = mbx_set_timeout;

  if (mbox->_get_refresh == NULL)
    mbox->_get_refresh = mbx_get_refresh;

  if (mbox->_set_refresh == NULL)
    mbox->_set_refresh = mbx_set_refresh;

  if (mbox->_get_size == NULL)
    mbox->_get_size = mbx_get_size;

  if (mbox->_set_notification == NULL)
    mbox->_set_notification = mbx_set_notification;

}


static int
mbx_open (mailbox_t mbox, int flag)
{
  errno = ENOSYS;
  return -1;
}

static int
mbx_close (mailbox_t mbox, int flag)
{
  errno = ENOSYS;
  return -1;
}

/* name */
static int
mbx_get_name (mailbox_t mbox, int *id, char *name, int offset, int n)
{
  errno = ENOSYS;
  return -1;
}

/* passwd */
static int
mbx_get_passwd (mailbox_t mbox, char *passwd, int offset, int len)
{
  errno = ENOSYS;
  return -1;
}

static int
mbx_get_mpasswd (mailbox_t mbox, char **passwd, int *len)
{
  int i;
  char *p;
  if ((i = mbox->_get_passwd (mbox, NULL, 0, 0)) <= 0
      || (p = malloc (i)) == NULL)
    {
      return -1;
    }
  *passwd = p;
  return *len = i;
}

static int
mbx_set_passwd (mailbox_t mbox, const char *passwd, int offset, int n)
{
  errno = ENOSYS;
  return -1;
}

/* deleting */
static int
mbx_delete (mailbox_t mbox, int id)
{
  errno = ENOSYS;
  return -1;
}

static int
mbx_undelete (mailbox_t mbox, int id)
{
  errno = ENOSYS;
  return -1;
}

static int
mbx_expunge (mailbox_t mbox)
{
  errno = ENOSYS;
  return -1;
}

static int
mbx_is_deleted (mailbox_t mbox, int id)
{
  errno = ENOSYS;
  return -1;
}


/* appending */
static int
mbx_new_msg (mailbox_t mbox, int *id)
{
  errno = ENOSYS;
  return -1;
}

static int
mbx_set_header (mailbox_t mbox, int id, const char *h,
		int offset, int n, int replace)
{
  errno = ENOSYS;
  return -1;
}

static int
mbx_set_body (mailbox_t mbox, int id, const char *b,
	      int offset, int n, int replace)
{
  errno = ENOSYS;
  return -1;
}

static int
mbx_append (mailbox_t mbox, int id)
{
  errno = ENOSYS;
  return -1;
}

static int
mbx_destroy_msg (mailbox_t mbox, int id)
{
  errno = ENOSYS;
  return -1;
}

/* reading */
static int
mbx_get_body (mailbox_t mbox, int id, char *b, int offset, int n)
{
  errno = ENOSYS;
  return -1;
}

static int
mbx_get_mbody (mailbox_t mbox, int id, char **body, int *len)
{
  int i;
  char *b;
  if ((i = mbox->_get_body (mbox, id, NULL, 0, 0)) <= 0
      || (b = malloc (i)) == NULL)
    {
      return -1;
    }
  *body = b;
  return *len = i;
}

static int
mbx_get_header (mailbox_t mbox, int id, char *h, int offset, int n)
{
  errno = ENOSYS;
  return -1;
}

static int
mbx_get_mheader (mailbox_t mbox, int id, char **header, int *len)
{
  int i;
  char *h;
  if ((i = mbox->_get_header (mbox, id, NULL, 0, 0)) <= 0
      || (h = malloc (i)) == NULL)
    {
      return -1;
    }
  *header = h;
  return *len = i;
}

/* locking */
static int
mbx_lock  (mailbox_t mbox, int flag)
{
  errno = ENOSYS;
  return -1;
}

static int
mbx_unlock (mailbox_t mbox)
{
  errno = ENOSYS;
  return -1;
}

/* misc */
static int
mbx_scan (mailbox_t mbox, int *msgs)
{
  errno = ENOSYS;
  return -1;
}

static int
mbx_is_updated (mailbox_t mbox)
{
  errno = ENOSYS;
  return -1;
}

static int
mbx_get_timeout (mailbox_t mbox, int *timeout)
{
  errno = ENOSYS;
  return -1;
}

static int
mbx_set_timeout (mailbox_t mbox, int timeout)
{
  errno = ENOSYS;
  return -1;
}

static int
mbx_get_refresh (mailbox_t mbox, int *refresh)
{
  errno = ENOSYS;
  return -1;
}

static int
mbx_set_refresh (mailbox_t mbox, int refresh)
{
  errno = ENOSYS;
  return -1;
}

static int
mbx_get_size (mailbox_t mbox, int id, size_t *size)
{
  errno = ENOSYS;
  return -1;
}

static int
mbx_set_notification (mailbox_t mbox, int (*func) (mailbox_t, void *arg))
{
  errno = ENOSYS;
  return -1;
}
