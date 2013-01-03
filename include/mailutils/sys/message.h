/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2001, 2005, 2007, 2010-2012 Free Software
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

#ifndef _MESSAGE0_H
#define _MESSAGE0_H

#include <mailutils/message.h>
#include <mailutils/mime.h>
#include <mailutils/monitor.h>

#include <sys/types.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MESSAGE_MODIFIED        0x10000
#define MESSAGE_INTERNAL_STREAM 0x20000
#define MESSAGE_MIME_OWNER      0x40000

struct _mu_message
{
  /* Reference count.  */
  int ref_count;

  /* Who is the owner.  */
  void *owner;

  mu_envelope_t envelope;
  mu_header_t header;
  mu_body_t body;

  int flags;
  mu_stream_t rawstream;
  mu_stream_t outstream;
  mu_attribute_t attribute;
  mu_monitor_t monitor;
  mu_mime_t mime;
  mu_observable_t observable;
  mu_mailbox_t mailbox;
  size_t orig_header_size;
  
  int (*_get_stream)     (mu_message_t, mu_stream_t *);
  int (*_get_uidl)       (mu_message_t, char *, size_t, size_t *);
  int (*_get_uid)        (mu_message_t, size_t *);
  int (*_get_qid)        (mu_message_t,	mu_message_qid_t *);
  int (*_get_num_parts)  (mu_message_t, size_t *);
  int (*_get_part)       (mu_message_t, size_t, mu_message_t *);
  int (*_imapenvelope)   (mu_message_t, struct mu_imapenvelope **);
  int (*_bodystructure)  (mu_message_t, struct mu_bodystructure **);
  int (*_is_multipart)   (mu_message_t, int *);
  int (*_lines)          (mu_message_t, size_t *, int);
  int (*_size)           (mu_message_t, size_t *);
};

#ifdef __cplusplus
}
#endif

#endif /* _MESSAGE0_H */
