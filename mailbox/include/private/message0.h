/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000 Free Software Foundation, Inc.

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

#include <attribute.h>
#include <header.h>
#include <message.h>
#include <mailbox.h>
#include <event.h>

#include <sys/types.h>
#include <stdio.h>

#ifdef _cplusplus
extern "C" {
#endif

#ifndef __P
# ifdef __STDC__
#  define __P(args) args
# else
#  define __P(args) ()
# endif
#endif /*__P */

/* The notion of body_t is not exported outside,
 * there was no need for yet another object.
 * since only floating messages need them. The functions
 * that manipulate those objects are static to message.c
 *
 */
struct _body
{
  FILE *file;
  void *owner;
};

typedef struct _body * body_t;

/* forward declaration */
struct _message
{
  header_t header;
  stream_t stream;
  body_t body;
  attribute_t attribute;
  size_t num;
  size_t size;

  /* who is the owner */
  void *owner;
  int ref_count;

  event_t event;
  size_t event_num;

  int (*_get_header)  __P ((message_t msg, header_t *hdr));
  int (*_set_header)  __P ((message_t msg, header_t hdr, void *owner));

  int (*_get_attribute)  __P ((message_t msg, attribute_t *attr));
  int (*_set_attribute)  __P ((message_t msg, attribute_t attr, void *owner));

  int (*_get_stream) __P ((message_t msg, stream_t *));
  int (*_set_stream) __P ((message_t msg, stream_t, void *owner));

  int (*_size)        __P ((message_t msg, size_t *size));

  int (*_clone)       __P ((message_t msg, message_t *cmsg));
};

#ifdef _cplusplus
}
#endif

extern void message_notification (message_t msg, size_t type);
#endif /* _MESSAGE_H */
