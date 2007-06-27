/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2005, 2007  Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

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

extern int mu_observer_create      (mu_observer_t *, void *owner);
extern void mu_observer_destroy    (mu_observer_t *, void *owner);
extern void * mu_observer_get_owner  (mu_observer_t);
extern int mu_observer_action      (mu_observer_t, size_t type);
extern int mu_observer_set_action  (mu_observer_t, 
                                 int (*_action) (mu_observer_t, size_t), 
                                 void *owner);
extern int mu_observer_set_destroy (mu_observer_t, 
                                 int (*_destroy) (mu_observer_t), void *owner);
extern int mu_observer_set_flags   (mu_observer_t, int flags);

extern int mu_observable_create    (mu_observable_t *, void *owner);
extern void mu_observable_destroy  (mu_observable_t *, void *owner);
extern void * mu_observable_get_owner (mu_observable_t);
extern int mu_observable_attach    (mu_observable_t, size_t type, mu_observer_t observer);
extern int mu_observable_detach    (mu_observable_t, mu_observer_t observer);
extern int mu_observable_notify    (mu_observable_t, int type);

#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_OBSERVER_H */
