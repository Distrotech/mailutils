/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

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

#ifndef _MAILUTILS_OBSERVER_H
#define _MAILUTILS_OBSERVER_H

#include <sys/types.h>
#include <mailutils/base.h>

#ifndef __P
#ifdef __STDC__
#define __P(args) args
#else
#define __P(args) ()
#endif
#endif /*__P */

#ifdef __cplusplus
extern "C" {
#endif

struct _observer;
struct _observable;
typedef struct _observer * observer_t;
typedef struct _observable * observable_t;

struct event
{
  int type;
  union
  {
    mailbox_t mailbox; /* For corrupted mailbox.  */
    int msgno;         /* For new message.  */
    int percentage;    /* Scan progress.  */
    message_t message; /* message sent.  */
  } data ;
};

#define MU_EVT_MESSAGE_ADD         0x010
#define MU_EVT_MAILBOX_PROGRESS    0x020
#define MU_EVT_AUTHORITY_FAILED    0x030
#define MU_EVT_MAILBOX_CORRUPT     0x040
#define MU_EVT_MAILER_MESSAGE_SENT 0x080

extern int observer_create       __P ((observer_t *, int (*action)
				       __P ((void *, struct event)), void *));

extern int observer_add_ref      __P ((observer_t));
extern int observer_release      __P ((observer_t));
extern int observer_destroy      __P ((observer_t));

extern int observer_action       __P ((observer_t, struct event));


extern int observable_create     __P ((observable_t *));
extern int observable_release    __P ((observable_t));
extern int observable_destroy    __P ((observable_t));

extern int observable_attach     __P ((observable_t, int, observer_t));
extern int observable_detach     __P ((observable_t, observer_t));
extern int observable_notify_all __P ((observable_t, struct event));

#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_OBSERVER_H */
