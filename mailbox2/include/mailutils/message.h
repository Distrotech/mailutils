/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA  */

#ifndef _MAILUTILS_MESSAGE_H
#define _MAILUTILS_MESSAGE_H

#include <mailutils/mu_features.h>
#include <mailutils/types.h>
#include <mailutils/envelope.h>
#include <mailutils/header.h>
#include <mailutils/body.h>
#include <mailutils/stream.h>
#include <mailutils/attribute.h>
#include <mailutils/mailbox.h>

#ifdef __cplusplus
extern "C" {
#endif

/* A message is considered to be a container for:
  header_t, body_t, and its attribute_t.  */

extern int  message_ref            __P ((message_t));
extern void message_destroy        __P ((message_t *));

extern int  message_is_modified    __P ((message_t));
extern int  message_clear_modified __P ((message_t));
extern int  message_get_mailbox    __P ((message_t, mailbox_t *));

extern int  message_get_envelope   __P ((message_t, envelope_t *));
extern int  message_get_header     __P ((message_t, header_t *));
extern int  message_get_body       __P ((message_t, body_t *));
extern int  message_get_attribute  __P ((message_t, attribute_t *));

extern int  message_get_stream     __P ((message_t, stream_t *));

extern int  message_get_property   __P ((message_t, property_t *));

extern int  message_is_multipart   __P ((message_t, int *));

extern int  message_get_size       __P ((message_t, size_t *));

extern int  message_get_lines      __P ((message_t, size_t *));

extern int  message_get_num_parts  __P ((message_t, size_t *nparts));

extern int  message_get_part       __P ((message_t, size_t, message_t *));

extern int  message_get_uidl       __P ((message_t, char *, size_t, size_t *));
extern int  message_get_uid        __P ((message_t, size_t *));

/* misc functions */
extern int  message_create_attachment __P ((const char *content_type,
					    const char *encoding,
					    const char *filename,
					   message_t *newmsg));
extern int  message_save_attachment __P ((message_t msg,
					  const char *filename, void **data));
extern int  message_encapsulate __P ((message_t msg, message_t *newmsg,
				      void **data));
extern int  message_unencapsulate __P ((message_t msg, message_t *newmsg,
					void **data));

#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_MESSAGE_H */
