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

#ifndef _MAILBOX0_H
#define _MAILBOX0_H

#include <mailbox.h>
#include <event.h>

#include <sys/types.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __P
# ifdef __STDC__
#  define __P(args) args
# else
#  define __P(args) ()
# endif
#endif /*__P */

struct _mailbox
{
  /* Data */
  char *name;
  auth_t auth;
  locker_t locker;
  url_t url;

  /* register events */
  event_t event;
  size_t event_num;

  /* Back pointer to the specific mailbox */
  void *data;

  /* Public methods */

  int  (*_init)            __P ((mailbox_t *, const char *));
  void (*_destroy)         __P ((mailbox_t *));

  int  (*_open)            __P ((mailbox_t, int flag));
  int  (*_close)           __P ((mailbox_t));

  /* messages */
  int  (*_get_message)     __P ((mailbox_t, size_t msgno, message_t *msg));
  int  (*_append_message)  __P ((mailbox_t, message_t msg));
  int  (*_messages_count)  __P ((mailbox_t, size_t *num));
  int  (*_expunge)         __P ((mailbox_t));

  int  (*_is_updated)      __P ((mailbox_t));

  int  (*_size)            __P ((mailbox_t, off_t *size));

  /* private */
  int  (*_delete)          __P ((mailbox_t, size_t msgno));
  int  (*_undelete)        __P ((mailbox_t, size_t msgno));
  int  (*_is_deleted)      __P ((mailbox_t, size_t msgno));
  int  (*_num_deleted)     __P ((mailbox_t, size_t *));
};

/* private */
extern int mailbox_delete         __P ((mailbox_t, size_t msgno));
extern int mailbox_undelete       __P ((mailbox_t, size_t msgno));
extern int mailbox_is_deleted     __P ((mailbox_t, size_t msgno));
extern int mailbox_num_deleted    __P ((mailbox_t, size_t *));

extern int mailbox_get_auth       __P ((mailbox_t mbox, auth_t *auth));
extern int mailbox_set_auth       __P ((mailbox_t mbox, auth_t auth));
extern int mailbox_get_locker     __P ((mailbox_t mbox, locker_t *locker));
extern int mailbox_set_locker     __P ((mailbox_t mbox, locker_t locker));
extern int mailbox_get_attribute  __P ((mailbox_t mbox, size_t msgno,
					attribute_t *attr));
extern int mailbox_set_attribute  __P ((mailbox_t mbox, size_t msgno,
					attribute_t attr));
extern void mailbox_notification   __P ((mailbox_t mbox, size_t type));


#ifdef __cplusplus
}
#endif

#endif /* _MAILBOX0_H */
