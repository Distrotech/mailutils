/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

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

#ifndef _MAILUTILS_MAILBOX_H
#define _MAILUTILS_MAILBOX_H

#include <sys/types.h>

#include <mailutils/debug.h>
#include <mailutils/folder.h>
#include <mailutils/locker.h>
#include <mailutils/message.h>
#include <mailutils/mu_features.h>
#include <mailutils/observer.h>
#include <mailutils/property.h>
#include <mailutils/stream.h>
#include <mailutils/types.h>
#include <mailutils/url.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const char *mu_path_maildir;

/* Constructor/destructor and possible types.  */
extern int  mailbox_create          __P ((mailbox_t *, const char *));
extern void mailbox_destroy         __P ((mailbox_t *));
extern int  mailbox_create_default  __P ((mailbox_t *, const char *));

extern int  mailbox_open            __P ((mailbox_t, int flag));
extern int  mailbox_close           __P ((mailbox_t));
extern int  mailbox_get_folder      __P ((mailbox_t, folder_t *));
extern int  mailbox_set_folder      __P ((mailbox_t, folder_t));
extern int  mailbox_uidvalidity     __P ((mailbox_t, unsigned long *));
extern int  mailbox_uidnext         __P ((mailbox_t, size_t *));

/* Messages.  */
extern int  mailbox_get_message     __P ((mailbox_t, size_t msgno, message_t *));
extern int  mailbox_append_message  __P ((mailbox_t, message_t));
extern int  mailbox_messages_count  __P ((mailbox_t, size_t *));
extern int  mailbox_messages_recent __P ((mailbox_t, size_t *));
extern int  mailbox_message_unseen  __P ((mailbox_t, size_t *));
extern int  mailbox_expunge         __P ((mailbox_t));
extern int  mailbox_save_attributes __P ((mailbox_t));

/* Update and scanning.  */
extern int  mailbox_get_size        __P ((mailbox_t, off_t *size));
extern int  mailbox_is_updated      __P ((mailbox_t));
extern int  mailbox_scan            __P ((mailbox_t, size_t no, size_t *count));

/* Mailbox Stream.  */
extern int  mailbox_set_stream      __P ((mailbox_t, stream_t));
extern int  mailbox_get_stream      __P ((mailbox_t, stream_t *));

/* Lock settings.  */
extern int  mailbox_get_locker      __P ((mailbox_t, locker_t *));
extern int  mailbox_set_locker      __P ((mailbox_t, locker_t));

/* Property.  */
extern int  mailbox_get_property    __P ((mailbox_t, property_t *));

/* URL.  */
extern int  mailbox_get_url         __P ((mailbox_t, url_t *));

/* For any debuging */
extern int  mailbox_has_debug       __P ((mailbox_t));
extern int  mailbox_get_debug       __P ((mailbox_t, mu_debug_t *));
extern int  mailbox_set_debug       __P ((mailbox_t, mu_debug_t));

/* Events.  */
extern int  mailbox_get_observable  __P ((mailbox_t, observable_t *));

#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_MAILBOX_H */
