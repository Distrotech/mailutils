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

#ifndef _MAILUTILS_HEADER_H
#define _MAILUTILS_HEADER_H

#include <mailutils/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MU_HEADER_UNIX_FROM                 "From "
#define MU_HEADER_RETURN_PATH               "Return-Path"
#define MU_HEADER_RECEIVED                  "Received"
#define MU_HEADER_DATE                      "Date"
#define MU_HEADER_FROM                      "From"
#define MU_HEADER_SENDER                    "Sender"
#define MU_HEADER_RESENT_FROM               "Resent-From"
#define MU_HEADER_SUBJECT                   "Subject"
#define MU_HEADER_SENDER                    "Sender"
#define MU_HEADER_RESENT_SENDER             "Resent-SENDER"
#define MU_HEADER_TO                        "To"
#define MU_HEADER_RESENT_TO                 "Resent-To"
#define MU_HEADER_CC                        "Cc"
#define MU_HEADER_RESENT_CC                 "Resent-Cc"
#define MU_HEADER_BCC                       "Bcc"
#define MU_HEADER_RESENT_BCC                "Resent-Bcc"
#define MU_HEADER_REPLY_TO                  "Reply-To"
#define MU_HEADER_RESENT_REPLY_TO           "Resent-Reply-To"
#define MU_HEADER_MESSAGE_ID                "Message-ID"
#define MU_HEADER_RESENT_MESSAGE_ID         "Resent-Message-ID"
#define MU_HEADER_IN_REPLY_TO               "In-Reply-To"
#define MU_HEADER_REFERENCE                 "Reference"
#define MU_HEADER_REFERENCES                "References"
#define MU_HEADER_ENCRYPTED                 "Encrypted"
#define MU_HEADER_PRECEDENCE                "Precedence"
#define MU_HEADER_STATUS                    "Status"
#define MU_HEADER_CONTENT_LENGTH            "Content-Length"
#define MU_HEADER_CONTENT_LANGUAGE          "Content-Language"
#define MU_HEADER_CONTENT_TRANSFER_ENCODING "Content-transfer-encoding"
#define MU_HEADER_CONTENT_ID                "Content-ID"
#define MU_HEADER_CONTENT_TYPE              "Content-Type"
#define MU_HEADER_CONTENT_DESCRIPTION       "Content-Description"
#define MU_HEADER_CONTENT_DISPOSITION       "Content-Disposition"
#define MU_HEADER_CONTENT_MD5               "Content-MD5"
#define MU_HEADER_MIME_VERSION              "MIME-Version"
#define MU_HEADER_X_UIDL                    "X-UIDL"
#define MU_HEADER_X_UID                     "X-UID"
#define MU_HEADER_X_IMAPBASE                "X-IMAPbase"
#define MU_HEADER_ENV_SENDER                "X-Envelope-Sender"
#define MU_HEADER_ENV_DATE                  "X-Envelope-Date"

/* Mime support header attribute */

extern int header_create __P ((header_t *, const char *, size_t, void *));
extern void header_destroy __P ((header_t *, void *));
extern void *header_get_owner __P ((header_t));

extern int header_is_modified __P ((header_t));
extern int header_clear_modified __P ((header_t));

/* Set and get field values by field name. */
extern int header_set_value __P ((header_t, const char *, const char *, int));
extern int header_get_value __P ((header_t, const char *, char *, size_t, size_t *));
extern int header_aget_value __P ((header_t, const char *, char **));

/* Get field values as an address_t. */
extern int header_get_address __P ((header_t, const char *, address_t *));

/* Set and get field values by field index (1-based). */
extern int header_get_field_count __P ((header_t, size_t *count));
extern int header_get_field_value __P ((header_t, size_t index, char *, size_t, size_t *));
extern int header_get_field_name __P ((header_t, size_t index, char *, size_t, size_t *));
extern int header_aget_field_value __P ((header_t, size_t index, char **));
extern int header_aget_field_name __P ((header_t, size_t index, char **));

extern int header_get_stream __P ((header_t, stream_t *));
extern int header_set_stream __P ((header_t, stream_t, void *));

extern int header_size __P ((header_t, size_t *));
extern int header_lines __P ((header_t, size_t *));


extern int header_set_set_value __P ((header_t,
				      int (*_set_value) __PMT ((header_t,
							        const char *,
							        const char *,
							        int)), void *));

extern int header_set_get_value __P ((header_t,
				      int (*_get_value) __PMT ((header_t,
							        const char *,
							        char *,
							        size_t,
							        size_t *)),
				      void *));
extern int header_set_get_fvalue __P ((header_t,
				       int (*_get_value) __PMT ((header_t,
							         const char *,
							         char *,
							         size_t,
							         size_t *)),
				       void *));

extern int header_set_size __P ((header_t, int (*_size)
				 __PMT ((header_t, size_t *)), void *));

extern int header_set_lines __P ((header_t,
				  int (*_lines) __PMT ((header_t,
						        size_t *)), void *));

extern int header_set_fill __P ((header_t,
				 int (*_fill) __PMT ((header_t, char *,
						      size_t, off_t,
						      size_t *)), void *owner));

#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_HEADER_H */
