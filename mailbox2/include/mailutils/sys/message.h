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

#ifndef _MAILUTILS_SYS_MESSAGE_H
#define _MAILUTILS_SYS_MESSAGE_H

#include <mailutils/message.h>

#ifndef __P
# ifdef __STDC__
#  define __P(args) args
# else
#  define __P(args) ()
# endif
#endif /* __P */

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
