/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2005, 2006,
   2007 Free Software Foundation, Inc.

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

#ifndef _MAILUTILS_MESSAGE_H
#define _MAILUTILS_MESSAGE_H

#include <mailutils/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* A message is considered to be a container for:
  mu_header_t, mu_body_t, and its mu_attribute_t.  */

extern int mu_message_create (mu_message_t *, void *owner);
extern void mu_message_destroy (mu_message_t *, void *owner);

extern int mu_message_create_copy (mu_message_t *to, mu_message_t from);

extern void * mu_message_get_owner (mu_message_t);
extern int mu_message_is_modified (mu_message_t);
extern int mu_message_clear_modified (mu_message_t);
extern int mu_message_get_mailbox (mu_message_t, mu_mailbox_t *);
extern int mu_message_set_mailbox (mu_message_t, mu_mailbox_t, void *);

extern int mu_message_ref (mu_message_t);
#define mu_message_unref(msg) mu_message_destroy (&msg, NULL)

extern int mu_message_get_envelope (mu_message_t, mu_envelope_t *);
extern int mu_message_set_envelope (mu_message_t, mu_envelope_t, void *);

extern int mu_message_get_header (mu_message_t, mu_header_t *);
extern int mu_message_set_header (mu_message_t, mu_header_t, void *);

extern int mu_message_get_body (mu_message_t, mu_body_t *);
extern int mu_message_set_body (mu_message_t, mu_body_t, void *);

extern int mu_message_get_stream (mu_message_t, mu_stream_t *);
extern int mu_message_set_stream (mu_message_t, mu_stream_t, void *);

extern int mu_message_get_attribute (mu_message_t, mu_attribute_t *);
extern int mu_message_set_attribute (mu_message_t, mu_attribute_t, void *);

extern int mu_message_get_observable (mu_message_t, mu_observable_t *);

extern int mu_message_is_multipart (mu_message_t, int *);
extern int mu_message_set_is_multipart (mu_message_t, 
					int (*_is_multipart) (mu_message_t,
							      int *), 
					void *);

extern int mu_message_size (mu_message_t, size_t *);
extern int mu_message_set_size (mu_message_t, 
				int (*_size) (mu_message_t, size_t *), 
				void *owner);

extern int mu_message_lines (mu_message_t, size_t *);
extern int mu_message_set_lines (mu_message_t, 
				 int (*_lines) (mu_message_t, size_t *),
				 void *owner);

extern int mu_message_get_num_parts (mu_message_t, size_t *nparts);
extern int mu_message_set_get_num_parts (mu_message_t, 
					 int (*_get_num_parts) (mu_message_t, 
								size_t *),
					 void *owner);

extern int mu_message_get_part (mu_message_t, size_t, mu_message_t *);
extern int mu_message_set_get_part (mu_message_t, 
				    int (*_get_part) (mu_message_t, size_t,
						      mu_message_t *), 
				    void *owner);

extern int mu_message_get_uidl (mu_message_t, char *, size_t, size_t *);
extern int mu_message_set_uidl (mu_message_t, 
				int (*_get_uidl) (mu_message_t,
						  char *, 
						  size_t, size_t *), 
				void *owner);
  
extern int mu_message_get_uid (mu_message_t, size_t *);
extern int mu_message_set_uid (mu_message_t, 
			       int (*_get_uid) (mu_message_t,
						size_t *),
			       void *owner);

extern int mu_message_get_qid (mu_message_t, mu_message_qid_t *);
extern int mu_message_set_qid (mu_message_t,
			       int (*_get_qid) (mu_message_t,
						mu_message_qid_t *),
			       void *owner);
  
/* misc functions */
extern int mu_message_create_attachment (const char *content_type,
					 const char *encoding,
					 const char *filename,
					 mu_message_t *newmsg);
extern int mu_message_save_attachment (mu_message_t msg,
				       const char *filename, void **data);
extern int mu_message_encapsulate (mu_message_t msg, mu_message_t *newmsg,
				   void **data);
extern int mu_message_unencapsulate (mu_message_t msg, mu_message_t *newmsg,
				     void **data);

extern int mu_message_get_attachment_name (mu_message_t, char *name,
					   size_t bufsz, size_t* sz);
extern int mu_message_aget_attachment_name (mu_message_t, char **name);

extern int mu_message_save_to_mailbox (mu_message_t msg, mu_ticket_t ticket,
				       mu_debug_t debug, const char *toname);


extern int mu_stream_to_message (mu_stream_t instream, mu_message_t *pmsg);

  
#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_MESSAGE_H */
