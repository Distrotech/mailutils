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

#ifndef _MAILUTILS_OBSERVER_H
#define _MAILUTILS_OBSERVER_H

#include <sys/types.h>

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
typedef struct _observer* observer_t;
typedef struct _observable* observable_t;

#define MU_EVT_MAILBOX_DESTROY     0x001
#define MU_EVT_FOLDER_DESTROY      0x002
#define MU_EVT_MAILER_DESTROY      0x004
#define MU_EVT_MESSAGE_DESTROY     0x008
#define MU_EVT_MESSAGE_ADD         0x010
#define MU_EVT_MAILBOX_PROGRESS    0x020
#define MU_EVT_AUTHORITY_FAILED    0x030
#define MU_EVT_MAILBOX_CORRUPT     0x040
#define MU_EVT_MAILER_MESSAGE_SENT 0x080

#define MU_OBSERVER_NO_CHECK 1

extern int observer_create      __P ((observer_t *, void *owner));
extern void observer_destroy    __P ((observer_t *, void *owner));
extern void * observer_get_owner  __P ((observer_t));
extern int observer_action      __P ((observer_t, size_t type));
extern int observer_set_action  __P ((observer_t, int (*_action) __P ((observer_t, size_t)), void *owner));
extern int observer_set_destroy __P ((observer_t, int (*_destroy) __P((observer_t)), void *owner));
extern int observer_set_flags   __P ((observer_t, int flags));

extern int observable_create    __P ((observable_t *, void *owner));
extern void observable_destroy  __P ((observable_t *, void *owner));
extern void * observable_get_owner __P ((observable_t));
extern int observable_attach    __P ((observable_t, size_t type, observer_t observer));
extern int observable_detach    __P ((observable_t, observer_t observer));
extern int observable_notify    __P ((observable_t, int type));

#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_OBSERVER_H */
