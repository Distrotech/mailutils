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

#ifndef _MAILBOX_H
# define _MAILBOX_H

#include <sys/types.h>

#include <url.h>
#include <message.h>
#include <attribute.h>
#include <auth.h>
#include <locker.h>

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

/* forward declaration */
struct _mailbox;
typedef struct _mailbox *mailbox_t;

/* constructor/destructor and possible types */
  extern int  mailbox_init __P ((mailbox_t *, const char *, int id));
extern void mailbox_destroy __P ((mailbox_t *));

/* flags for mailbox_open () */
#define MU_MAILBOX_RDONLY ((int)1)
#define MU_MAILBOX_WRONLY (MU_MAILBOX_RDONLY << 1)
#define MU_MAILBOX_RDWR   (MU_MAILBOX_WRONLY << 1)
#define MU_MAILBOX_APPEND (MU_MAILBOX_RDWR << 1)
#define MU_MAILBOX_CREAT  (MU_MAILBOX_APPEND << 1)
#define MU_MAILBOX_NONBLOCK  (MU_MAILBOX_CREAT << 1)

extern int mailbox_open     __P ((mailbox_t, int flag));
extern int mailbox_close    __P ((mailbox_t));

/* messages */
extern int mailbox_get_message __P ((mailbox_t, size_t msgno, message_t *msg));
extern int mailbox_append_message __P ((mailbox_t, message_t msg));
extern int mailbox_messages_count __P ((mailbox_t, size_t *num));
extern int mailbox_expunge  __P ((mailbox_t));

/* Lock settings */
extern int mailbox_get_locker      __P ((mailbox_t, locker_t *locker));
extern int mailbox_set_locker      __P ((mailbox_t, locker_t locker));

/* Authentication */
extern int mailbox_get_auth      __P ((mailbox_t, auth_t *auth));
extern int mailbox_set_auth      __P ((mailbox_t, auth_t auth));

/* update and scanning*/
extern int mailbox_is_updated    __P ((mailbox_t));
extern int mailbox_scan    __P ((mailbox_t, size_t msgno, size_t *count));

/* mailbox size ? */
extern int mailbox_size          __P ((mailbox_t, off_t size));

extern int mailbox_get_url       __P ((mailbox_t, url_t *));

/* events */
#define MU_EVT_MBX_DESTROY      1
#define MU_EVT_MBX_CORRUPTED    2
#define MU_EVT_MBX_MSG_ADD      4
#define MU_EVT_MBX_PROGRESS     8

extern int mailbox_register __P ((mailbox_t mbox, size_t type,
				  int (*action) (size_t type, void *arg),
				  void *arg));
extern int mailbox_deregister __P ((mailbox_t mbox, void *action));

#ifdef __cplusplus
}
#endif

#endif /* _MAILBOX_H */
