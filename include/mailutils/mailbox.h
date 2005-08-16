/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2005  Free Software Foundation, Inc.

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

#ifndef _MAILUTILS_MAILBOX_H
#define _MAILUTILS_MAILBOX_H

#include <sys/types.h>

#include <mailutils/types.h>

#ifdef __cplusplus
extern "C" {
#endif

int mu_set_mail_directory (const char *p);
void mu_set_folder_directory (const char *p);
const char *mu_mail_directory (void);
const char *mu_folder_directory (void);
int mu_construct_user_mailbox_url (char **pout, const char *name);
  
/* Constructor/destructor and possible types.  */
extern int  mailbox_create          (mailbox_t *, const char *);
extern void mailbox_destroy         (mailbox_t *);
extern int  mailbox_create_default  (mailbox_t *, const char *);

extern int  mailbox_open            (mailbox_t, int flag);
extern int  mailbox_close           (mailbox_t);
extern int  mailbox_flush           (mailbox_t mbox, int expunge);
extern int  mailbox_get_folder      (mailbox_t, folder_t *);
extern int  mailbox_set_folder      (mailbox_t, folder_t);
extern int  mailbox_uidvalidity     (mailbox_t, unsigned long *);
extern int  mailbox_uidnext         (mailbox_t, size_t *);

/* Messages.  */
extern int  mailbox_get_message     (mailbox_t, size_t msgno, message_t *);
extern int  mailbox_append_message  (mailbox_t, message_t);
extern int  mailbox_messages_count  (mailbox_t, size_t *);
extern int  mailbox_messages_recent (mailbox_t, size_t *);
extern int  mailbox_message_unseen  (mailbox_t, size_t *);
extern int  mailbox_expunge         (mailbox_t);
extern int  mailbox_save_attributes (mailbox_t);

/* Update and scanning.  */
extern int  mailbox_get_size        (mailbox_t, off_t *size);
extern int  mailbox_is_updated      (mailbox_t);
extern int  mailbox_scan            (mailbox_t, size_t no, size_t *count);

/* Mailbox Stream.  */
extern int  mailbox_set_stream      (mailbox_t, stream_t);
extern int  mailbox_get_stream      (mailbox_t, stream_t *);

/* Lock settings.  */
extern int  mailbox_get_locker      (mailbox_t, locker_t *);
extern int  mailbox_set_locker      (mailbox_t, locker_t);

/* Property.  */
extern int  mailbox_get_flags       (mailbox_t, int *);
extern int  mailbox_get_property    (mailbox_t, property_t *);

/* URL.  */
extern int  mailbox_get_url         (mailbox_t, url_t *);

/* For any debuging */
extern int  mailbox_has_debug       (mailbox_t);
extern int  mailbox_get_debug       (mailbox_t, mu_debug_t *);
extern int  mailbox_set_debug       (mailbox_t, mu_debug_t);

/* Events.  */
extern int  mailbox_get_observable  (mailbox_t, observable_t *);

/* Locking */  
extern int mailbox_lock (mailbox_t mbox);
extern int mailbox_unlock (mailbox_t mbox);
  
#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_MAILBOX_H */
