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
#include <mailutils/mu_features.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _observer;
typedef struct _observer * observer_t;

struct event
{
  int type;
  union
  {
    void *mailbox; /* For corrupted mailbox.  */
    int msgno;         /* For new message.  */
    int percentage;    /* Scan progress.  */
    void *message; /* message sent.  */
  } data ;
};

#define MU_EVT_MESSAGE_ADD         0x010
#define MU_EVT_MAILBOX_PROGRESS    0x020
#define MU_EVT_AUTHORITY_FAILED    0x030
#define MU_EVT_MAILBOX_CORRUPT     0x040
#define MU_EVT_MAILER_MESSAGE_SENT 0x080

extern int  observer_create  __P ((observer_t *, int (*action)
				   __P ((void *, struct event)), void *));

extern int  observer_ref __P ((observer_t));
extern void observer_destroy __P ((observer_t *));
extern int  observer_action  __P ((observer_t, struct event));

#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_OBSERVER_H */
