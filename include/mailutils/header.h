/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU Library General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifndef _MAILUTILS_HEADER_H
#define _MAILUTILS_HEADER_H

#include <sys/types.h>
#include <mailutils/stream.h>

#ifndef __P
#ifdef __STDC__
#define __P(args) args
#else
#define __P(args) ()
#endif
#endif /*__P */

#ifdef _cplusplus
extern "C" {
#endif

#define MU_HEADER_UNIX_FROM             "From "
#define MU_HEADER_RETURN_PATH           "Return-Path"
#define MU_HEADER_RECEIVED              "Received"
#define MU_HEADER_DATE                  "Date"
#define MU_HEADER_FROM                  "From"
#define MU_HEADER_RESENT_FROM           "Resent-From"
#define MU_HEADER_SUBJECT               "Subject"
#define MU_HEADER_SENDER                "Sender"
#define MU_HEADER_RESENT_SENDER         "Resent-SENDER"
#define MU_HEADER_TO                    "To"
#define MU_HEADER_RESENT_TO             "Resent-To"
#define MU_HEADER_CC                    "Cc"
#define MU_HEADER_RESENT_CC             "Resent-Cc"
#define MU_HEADER_BCC                   "Bcc"
#define MU_HEADER_RESENT_BCC            "Resent-Bcc"
#define MU_HEADER_REPLY_TO              "Reply-To"
#define MU_HEADER_RESENT_REPLY_TO       "Resent-Reply-To"
#define MU_HEADER_MESSAGE_ID            "Message-ID"
#define MU_HEADER_RESENT_MESSAGE_ID     "Resent-Message-ID"
#define MU_HEADER_IN_REPLY_TO           "In-Reply-To"
#define MU_HEADER_ENCRYPTED             "Encrypted"
#define MU_HEADER_PRECEDENCE            "Precedence"
#define MU_HEADER_STATUS                "Status"
#define MU_HEADER_CONTENT_LENGTH        "Content-Length"
#define MU_HEADER_CONTENT_TYPE          "Content-Type"
#define MU_HEADER_CONTENT_ENCODING      "Content-transfer-encoding"
#define MU_HEADER_MIME_VERSION          "MIME-Version"

/* Mime support header attribute */

/* forward declaration */
struct _header;
typedef struct _header * header_t;

extern int header_create        __P ((header_t *, const char *,
				      size_t, void *));
extern void header_destroy      __P ((header_t *, void *));
extern void * header_get_owner  __P ((header_t));

extern int header_is_modified   __P ((header_t));

extern int header_set_value     __P ((header_t, const char *,
				      const char *, int));
extern int header_set_set_value __P ((header_t, int (*_set_value)
				      __P ((header_t, const char *,
					    const char *, int)), void *));

extern int header_get_value     __P ((header_t, const char *, char *,
				      size_t, size_t *));
extern int header_set_get_value __P ((header_t, int (*_get_value)
				      __P ((header_t, const char *, char *,
					    size_t, size_t *)), void *));
extern int header_set_get_fvalue __P ((header_t, int (*_get_value)
				       __P ((header_t, const char *, char *,
					     size_t, size_t *)), void *));

extern int header_get_stream    __P ((header_t, stream_t *));
extern int header_set_stream    __P ((header_t, stream_t, void *));

extern int header_size          __P ((header_t, size_t *));
extern int header_set_size      __P ((header_t, int (*_size)
				      __P ((header_t, size_t *)), void *));

extern int header_lines         __P ((header_t, size_t *));
extern int header_set_lines     __P ((header_t,
				      int (*_lines) __P ((header_t,
							  size_t *)),
				      void *));

extern int header_set_fill      __P ((header_t,
				      int (*_fill) __P ((header_t, char *,
							 size_t, off_t,
							 size_t *)),
				      void *owner));
#ifdef _cplusplus
}
#endif

#endif /* _MAILUTILS_HEADER_H */
