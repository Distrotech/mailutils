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

#ifndef _MAILUTILS_SYS_MESSAGE_H
#define _MAILUTILS_SYS_MESSAGE_H

#ifdef DMALLOC
# include <dmalloc.h>
#endif

#include <mailutils/message.h>

#ifdef __cplusplus
extern "C" {
#endif

/* A message is considered to be a container for:
  header_t, body_t, and its attribute_t.  */

struct _message_vtable
{
  int  (*ref)            __P ((message_t));
  void (*destroy)        __P ((message_t *));

  int  (*is_modified)    __P ((message_t));
  int  (*clear_modified) __P ((message_t));
  int  (*get_mailbox)    __P ((message_t, mailbox_t *));

  int  (*get_envelope)   __P ((message_t, envelope_t *));
  int  (*get_header)     __P ((message_t, header_t *));
  int  (*get_body)       __P ((message_t, body_t *));
  int  (*get_attribute)  __P ((message_t, attribute_t *));

  int  (*get_stream)     __P ((message_t, stream_t *));

  int  (*get_property)   __P ((message_t, property_t *));

  int  (*is_multipart)   __P ((message_t, int *));

  int  (*get_size)       __P ((message_t, size_t *));

  int  (*get_lines)      __P ((message_t, size_t *));

  int  (*get_num_parts)  __P ((message_t, size_t *nparts));

  int  (*get_part)       __P ((message_t, size_t, message_t *));

  int  (*get_uidl)       __P ((message_t, char *, size_t, size_t *));
  int  (*get_uid)        __P ((message_t, size_t *));
};

struct _message
{
  struct _message_vtable *vtable;
};


#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_SYS_MESSAGE_H */
