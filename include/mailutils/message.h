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

#ifndef _MAILUTILS_MESSAGE_H
#define _MAILUTILS_MESSAGE_H

#include <mailutils/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* A message is considered to be a container for:
  header_t, body_t, and its attribute_t.  */

extern int message_create           (message_t *, void *owner);
extern void message_destroy         (message_t *, void *owner);

extern int message_create_copy      (message_t *to, message_t from);

extern void * message_get_owner     (message_t);
extern int message_is_modified      (message_t);
extern int message_clear_modified   (message_t);
extern int message_get_mailbox      (message_t, mailbox_t *);
extern int message_set_mailbox      (message_t, mailbox_t, void *);

extern int message_ref              (message_t);
#define message_unref(msg)          message_destroy (&msg, NULL)

extern int message_get_envelope     (message_t, envelope_t *);
extern int message_set_envelope     (message_t, envelope_t, void *);

extern int message_get_header       (message_t, header_t *);
extern int message_set_header       (message_t, header_t, void *);

extern int message_get_body         (message_t, body_t *);
extern int message_set_body         (message_t, body_t, void *);

extern int message_get_stream       (message_t, stream_t *);
extern int message_set_stream       (message_t, stream_t, void *);

extern int message_get_attribute    (message_t, attribute_t *);
extern int message_set_attribute    (message_t, attribute_t, void *);

extern int message_get_observable   (message_t, observable_t *);

extern int message_is_multipart     (message_t, int *);
extern int message_set_is_multipart (message_t, 
                                     int (*_is_multipart) (message_t, int *), 
                                     void *);

extern int message_size             (message_t, size_t *);
extern int message_set_size         (message_t, 
                                     int (*_size) (message_t, size_t *), 
                                     void *owner);

extern int message_lines            (message_t, size_t *);
extern int message_set_lines        (message_t, 
                                     int (*_lines) (message_t, size_t *),
				     void *owner);

extern int message_get_num_parts    (message_t, size_t *nparts);
extern int message_set_get_num_parts (message_t, 
                                      int (*_get_num_parts) (message_t, 
                                                             size_t *),
				      void *owner);

extern int message_get_part         (message_t, size_t, message_t *);
extern int message_set_get_part     (message_t, 
                                     int (*_get_part) (message_t, size_t,
						       message_t *), 
                                     void *owner);

extern int message_get_uidl         (message_t, char *, size_t, size_t *);
extern int message_set_uidl         (message_t, 
                                     int (*_get_uidl) (message_t, char *, 
                                                       size_t, size_t *), 
                                     void *owner);
extern int message_get_uid          (message_t, size_t *);
extern int message_set_uid          (message_t, 
                                     int (*_get_uid) (message_t, size_t *),
				     void *owner);

/* misc functions */
extern int message_create_attachment (const char *content_type,
				      const char *encoding,
				      const char *filename,
				      message_t *newmsg);
extern int message_save_attachment (message_t msg,
				    const char *filename, void **data);
extern int message_encapsulate (message_t msg, message_t *newmsg, void **data);
extern int message_unencapsulate (message_t msg, message_t *newmsg, void **data);

extern int message_get_attachment_name (message_t, char *name, size_t bufsz, size_t* sz);
extern int message_aget_attachment_name (message_t, char **name);

extern int message_save_to_mailbox (message_t msg, ticket_t ticket,
                                    mu_debug_t debug, const char *toname);


#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_MESSAGE_H */
