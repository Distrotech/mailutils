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

/* forward declaration */
struct _message
{
  header_t header;
  stream_t stream;
  body_t body;
  attribute_t attribute;

  /* who is the owner */
  void *owner;

  event_t event;
  size_t event_num;

  int (*_from)       __P ((message_t msg, char *, size_t, size_t *));
  int (*_received)   __P ((message_t msg, char *, size_t, size_t *));

};

#ifdef _cplusplus
}
#endif

extern void message_notification (message_t msg, size_t type);
#endif /* _MESSAGE_H */
