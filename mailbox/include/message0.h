/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Library Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifndef _MESSAGE0_H
#define _MESSAGE0_H

#ifdef DMALLOC
#  include <dmalloc.h>
#endif

#include <mailutils/message.h>
#include <mailutils/mime.h>
#include <mailutils/monitor.h>

#include <sys/types.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _message
{
  /* Who is the owner.  */
  void *owner;

  envelope_t envelope;
  header_t header;
  body_t body;

  int flags;
  stream_t stream;
  attribute_t attribute;
  monitor_t monitor;
  mime_t mime;
  observable_t observable;
  mailbox_t mailbox;

  /* Reference count.  */
  int ref;

  /* Holder for message_write. */
  size_t hdr_buflen;
  int hdr_done;

  int (*_get_uidl)       __P ((message_t, char *, size_t, size_t *));
  int (*_get_uid)        __P ((message_t, size_t *));
  int (*_get_num_parts)  __P ((message_t, size_t *));
  int (*_get_part)       __P ((message_t, size_t, message_t *));
  int (*_is_multipart)   __P ((message_t, int *));
  int (*_lines)          __P ((message_t, size_t *));
  int (*_size)           __P ((message_t, size_t *));
};

#ifdef __cplusplus
}
#endif

#endif /* _MESSAGE0_H */
