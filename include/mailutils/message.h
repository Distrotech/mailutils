/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Library Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with GNU Mailutils; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifndef _MAILUTILS_MESSAGE_H
#define _MAILUTILS_MESSAGE_H

#include <mailutils/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* A message is considered to be a container for:
  header_t, body_t, and its attribute_t.  */

extern int message_create           __P ((message_t *, void *owner));
extern void message_destroy         __P ((message_t *, void *owner));

extern int message_create_copy      __P ((message_t *to, message_t from));

extern void * message_get_owner     __P ((message_t));
extern int message_is_modified      __P ((message_t));
extern int message_clear_modified   __P ((message_t));
extern int message_get_mailbox      __P ((message_t, mailbox_t *));
extern int message_set_mailbox      __P ((message_t, mailbox_t, void *));

extern int message_ref              __P ((message_t));
#define message_unref(msg)          message_destroy (&msg, NULL)

extern int message_get_envelope     __P ((message_t, envelope_t *));
extern int message_set_envelope     __P ((message_t, envelope_t, void *));

extern int message_get_header       __P ((message_t, header_t *));
extern int message_set_header       __P ((message_t, header_t, void *));

extern int message_get_body         __P ((message_t, body_t *));
extern int message_set_body         __P ((message_t, body_t, void *));

extern int message_get_stream       __P ((message_t, stream_t *));
extern int message_set_stream       __P ((message_t, stream_t, void *));

extern int message_get_attribute    __P ((message_t, attribute_t *));
extern int message_set_attribute    __P ((message_t, attribute_t, void *));

extern int message_get_observable   __P ((message_t, observable_t *));

extern int message_is_multipart     __P ((message_t, int *));
extern int message_set_is_multipart __P ((message_t, int (*_is_multipart)
					  __PMT ((message_t, int *)), void *));

extern int message_size             __P ((message_t, size_t *));
extern int message_set_size         __P ((message_t, int (*_size)
					  __PMT ((message_t, size_t *)),
					   void *owner));

extern int message_lines            __P ((message_t, size_t *));
extern int message_set_lines        __P ((message_t, int (*_lines)
					  __PMT ((message_t, size_t *)),
					  void *owner));

extern int message_get_num_parts    __P ((message_t, size_t *nparts));
extern int message_set_get_num_parts __P ((message_t, int (*_get_num_parts)
					   __PMT ((message_t, size_t *)),
					   void *owner));

extern int message_get_part         __P ((message_t, size_t, message_t *));
extern int message_set_get_part     __P ((message_t, int (*_get_part)
					  __PMT ((message_t, size_t,
						  message_t *)), void *owner));

extern int message_get_uidl         __P ((message_t, char *, size_t, size_t *));
extern int message_set_uidl         __P ((message_t, int (*_get_uidl)
					  __PMT ((message_t, char *, size_t,
					  	  size_t *)), void *owner));
extern int message_get_uid          __P ((message_t, size_t *));
extern int message_set_uid          __P ((message_t, int (*_get_uid)
					  __PMT ((message_t, size_t *)),
					  void *owner));

/* misc functions */
extern int message_create_attachment __P ((const char *content_type,
					   const char *encoding,
					   const char *filename,
					   message_t *newmsg));
extern int message_save_attachment __P ((message_t msg,
					 const char *filename, void **data));
extern int message_encapsulate __P ((message_t msg, message_t *newmsg,
				     void **data));
extern int message_unencapsulate __P ((message_t msg, message_t *newmsg,
				       void **data));

extern int message_get_attachment_name __P ((message_t, char *name, size_t bufsz, size_t* sz));
extern int message_aget_attachment_name __P ((message_t, char **name));

extern int message_save_to_mailbox __P ((message_t msg, ticket_t ticket,
      mu_debug_t debug, const char *toname));


#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_MESSAGE_H */
