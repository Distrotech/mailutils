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

#ifndef _MESSAGE_H
#define _MESSAGE_H

#include <header.h>
#include <attribute.h>
#include <io.h>

#include <sys/types.h>

#ifndef __P
# ifdef __STDC__
#  define __P(args) args
# else
#  define __P(args) ()
# endif
#endif /* __P */

#ifdef _cplusplus
extern "C" {
#endif

/* forward declaration */
struct _message;
typedef struct _message *message_t;

/* A message is considered to be a container for:
 * header_t, body_t, and its attribute_t.
 * The notion of body_t is not visible/exported, since
 * they are alway tied to a floating message, there was no
 * need to create yet another object, getting the {i,o}stream_t
 * was enough.
 */

extern int message_init        __P ((message_t *, void *owner));
extern void message_destroy    __P ((message_t *, void *owner));

extern int message_get_header  __P ((message_t, header_t *));
extern int message_set_header  __P ((message_t, header_t, void *owner));

extern int message_get_stream __P ((message_t, stream_t *));
extern int message_set_stream __P ((message_t, stream_t, void *owner));

extern int message_is_multipart __P ((message_t));

extern int message_get_size     __P ((message_t, size_t *));
extern int message_set_size     __P ((message_t, size_t, void *owner));

extern int message_get_attribute __P ((message_t, attribute_t *));
extern int message_set_attribute __P ((message_t, attribute_t, void *owner));

extern int message_clone __P ((message_t));

/* events */
#define MU_EVT_MSG_DESTROY 32
extern int message_register __P ((message_t msg, size_t type,
				  int (*action) (size_t typ, void *arg),
				  void *arg));
extern int message_deregister __P ((message_t msg, void *action));
#ifdef _cplusplus
}
#endif

#endif /* _MESSAGE_H */
