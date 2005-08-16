/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2005  Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library; if not, write to the
   Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301 USA */

#ifndef _MAILUTILS_OBSERVER_H
#define _MAILUTILS_OBSERVER_H

#include <mailutils/types.h>

#ifdef __cplusplus
extern "C" {
#endif

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

extern int observer_create      (observer_t *, void *owner);
extern void observer_destroy    (observer_t *, void *owner);
extern void * observer_get_owner  (observer_t);
extern int observer_action      (observer_t, size_t type);
extern int observer_set_action  (observer_t, 
                                 int (*_action) (observer_t, size_t), 
                                 void *owner);
extern int observer_set_destroy (observer_t, 
                                 int (*_destroy) (observer_t), void *owner);
extern int observer_set_flags   (observer_t, int flags);

extern int observable_create    (observable_t *, void *owner);
extern void observable_destroy  (observable_t *, void *owner);
extern void * observable_get_owner (observable_t);
extern int observable_attach    (observable_t, size_t type, observer_t observer);
extern int observable_detach    (observable_t, observer_t observer);
extern int observable_notify    (observable_t, int type);

#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_OBSERVER_H */
