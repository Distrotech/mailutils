/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2001, 2005-2007, 2010-2012 Free Software
   Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library.  If not, see 
   <http://www.gnu.org/licenses/>. */

#ifndef _MAILUTILS_MESSAGE_H
#define _MAILUTILS_MESSAGE_H

#include <mailutils/types.h>
#include <mailutils/datetime.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MU_SCAN_SEEK  0x01
#define MU_SCAN_SIZE  0x02

struct mu_imapenvelope
{
  struct tm date;
  struct mu_timezone tz;
  char *subject;
  mu_address_t from;
  mu_address_t sender;
  mu_address_t reply_to;
  mu_address_t to;
  mu_address_t cc;
  mu_address_t bcc;
  char *in_reply_to;
  char *message_id;
};

enum mu_message_type
  {
    mu_message_other,
    mu_message_text,     /* text/plain */
    mu_message_rfc822,   /* message/rfc822 */
    mu_message_multipart /* multipart/mixed */
  };
  
struct mu_bodystructure
{
  enum mu_message_type body_message_type;
  char *body_type;
  char *body_subtype;
  mu_assoc_t body_param;
  char *body_id;
  char *body_descr;
  char *body_encoding;
  size_t body_size;
  /* Optional */
  char *body_md5;
  char *body_disposition;
  mu_assoc_t body_disp_param;
  char *body_language;
  char *body_location;
  union
  {
    struct
    {
      size_t body_lines;
    } text;
    struct 
    {
      struct mu_imapenvelope *body_env;
      struct mu_bodystructure *body_struct;
      size_t body_lines;
    } rfc822;
    struct
    {
      mu_list_t body_parts;
    } multipart;
  } v;
};

struct mu_message_scan
{
  int flags;
  mu_off_t message_start;
  mu_off_t message_size;

  mu_off_t body_start;
  mu_off_t body_end;
  size_t header_lines;
  size_t body_lines;
  int attr_flags;
  unsigned long uidvalidity;
};

int mu_stream_scan_message (mu_stream_t stream, struct mu_message_scan *sp);
  
/* A message is considered to be a container for:
  mu_header_t, mu_body_t, and its mu_attribute_t.  */

extern int mu_message_create (mu_message_t *, void *owner);
extern void mu_message_destroy (mu_message_t *, void *owner);

extern int mu_message_create_copy (mu_message_t *to, mu_message_t from);

extern void *mu_message_get_owner (mu_message_t);

#define MU_MSG_ATTRIBUTE_MODIFIED 0x01
#define MU_MSG_HEADER_MODIFIED    0x02 
#define MU_MSG_BODY_MODIFIED      0x04

extern int mu_message_is_modified (mu_message_t);
extern int mu_message_clear_modified (mu_message_t);
extern int mu_message_get_mailbox (mu_message_t, mu_mailbox_t *);
extern int mu_message_set_mailbox (mu_message_t, mu_mailbox_t, void *);

extern void mu_message_ref (mu_message_t);
extern void mu_message_unref (mu_message_t);

extern int mu_message_get_envelope (mu_message_t, mu_envelope_t *);
extern int mu_message_set_envelope (mu_message_t, mu_envelope_t, void *);

extern int mu_message_get_header (mu_message_t, mu_header_t *);
extern int mu_message_set_header (mu_message_t, mu_header_t, void *);

extern int mu_message_get_body (mu_message_t, mu_body_t *);
extern int mu_message_set_body (mu_message_t, mu_body_t, void *);

extern int mu_message_get_stream (mu_message_t, mu_stream_t *)
                                   __attribute__((deprecated));
extern int mu_message_get_streamref (mu_message_t, mu_stream_t *);

extern int mu_message_set_stream (mu_message_t, mu_stream_t, void *)
  __attribute__((deprecated));

extern int mu_message_get_attribute (mu_message_t, mu_attribute_t *);
extern int mu_message_set_attribute (mu_message_t, mu_attribute_t, void *);

extern int mu_message_get_observable (mu_message_t, mu_observable_t *);

extern int mu_message_set_get_stream (mu_message_t,
				      int (*) (mu_message_t, mu_stream_t *),
				      void *);
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
extern int mu_message_quick_lines (mu_message_t, size_t *);
extern int mu_message_set_lines (mu_message_t, 
				 int (*_lines) (mu_message_t, size_t *, int),
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

extern int mu_message_get_imapenvelope (mu_message_t, struct mu_imapenvelope **);
extern void mu_message_imapenvelope_free (struct mu_imapenvelope *);
extern int mu_message_set_imapenvelope (mu_message_t,
   int (*_imapenvelope) (mu_message_t, struct mu_imapenvelope **),
   void *owner);

extern void mu_bodystructure_free (struct mu_bodystructure *);
extern void mu_list_free_bodystructure (void *item);

extern int mu_message_get_bodystructure (mu_message_t,
					 struct mu_bodystructure **);
extern int mu_message_set_bodystructure (mu_message_t msg,
      int (*_bodystructure) (mu_message_t, struct mu_bodystructure **),
      void *owner);
  
/* misc functions */
extern int mu_message_create_attachment (const char *content_type,
					 const char *encoding,
					 const char *filename,
					 mu_message_t *newmsg);
extern int mu_message_save_attachment (mu_message_t msg,
				       const char *filename,
				       mu_mime_io_buffer_t buf);
extern int mu_message_encapsulate (mu_message_t msg, mu_message_t *newmsg,
				   mu_mime_io_buffer_t buf);
extern int mu_message_unencapsulate (mu_message_t msg, mu_message_t *newmsg,
				     mu_mime_io_buffer_t buf);

extern int mu_mime_io_buffer_create (mu_mime_io_buffer_t *pinfo);
extern void mu_mime_io_buffer_destroy (mu_mime_io_buffer_t *pinfo);
  
extern int mu_mime_io_buffer_set_charset (mu_mime_io_buffer_t info,
					  const char *charset);
extern void mu_mime_io_buffer_sget_charset (mu_mime_io_buffer_t info,
					    const char **charset);
extern int mu_mime_io_buffer_aget_charset (mu_mime_io_buffer_t info,
					   const char **charset);

  
extern int mu_mimehdr_get_disp (const char *str, char *buf, size_t bufsz,
				size_t *retsz);
extern int mu_mimehdr_aget_disp (const char *str, char **pvalue);
extern int mu_mimehdr_get_param (const char *str, const char *param,
				 char *buf, size_t bufsz, size_t *retsz);
extern int mu_mimehdr_aget_param (const char *str, const char *param,
				  char **pval);
extern int mu_mimehdr_aget_decoded_param (const char *str, const char *param,
					  const char *charset, 
					  char **pval, char **plang);
  
extern int mu_message_get_attachment_name (mu_message_t, char *name,
					   size_t bufsz, size_t* sz);
extern int mu_message_aget_attachment_name (mu_message_t, char **name);
extern int mu_message_aget_decoded_attachment_name (mu_message_t msg,
						    const char *charset,
						    char **name,
						    char **plang);

extern int mu_message_save_to_mailbox (mu_message_t msg, const char *toname,
				       int perms);


extern int mu_message_from_stream_with_envelope (mu_message_t *pmsg,
						 mu_stream_t instream,
						 mu_envelope_t env);
extern int mu_stream_to_message (mu_stream_t instream, mu_message_t *pmsg);

#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_MESSAGE_H */
