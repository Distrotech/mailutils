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

#include <sys/types.h>
#include <mailutils/header.h>
#include <mailutils/body.h>
#include <mailutils/attribute.h>
#include <mailutils/stream.h>


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
 */

extern int message_create            __P ((message_t *, void *owner));
extern void message_destroy          __P ((message_t *, void *owner));

extern int message_get_header        __P ((message_t, header_t *));
extern int message_set_header        __P ((message_t, header_t, void *owner));

extern int message_get_body          __P ((message_t, body_t *));
extern int message_set_body          __P ((message_t, body_t, void *owner));

extern int message_get_stream        __P ((message_t, stream_t *));
extern int message_set_stream        __P ((message_t, stream_t, void *owner));

extern int message_is_multipart      __P ((message_t, int *));
extern int message_set_is_multipart  __P ((message_t, int (*_is_multipart)
					   __P ((message_t, int *)), void *));

extern int message_size              __P ((message_t, size_t *));
extern int message_set_size          __P ((message_t, int (*_size)
					   __P ((message_t, size_t *)),
					   void *owner));

extern int message_lines             __P ((message_t, size_t *));
extern int message_set_lines         __P ((message_t, int (*_lines)
					   __P ((message_t, size_t *)),
					   void *owner));

extern int message_from              __P ((message_t, char *, size_t, size_t *));
extern int message_set_from          __P ((message_t, int (*_from)
					   __P ((message_t, char *, size_t,
						 size_t *)), void *owner));

extern int message_received          __P ((message_t, char *, size_t, size_t *));
extern int message_set_received      __P ((message_t, int (*_received)
					   __P ((message_t, char *, size_t,
						 size_t *)), void *owner));

extern int message_get_attribute     __P ((message_t, attribute_t *));
extern int message_set_attribute     __P ((message_t, attribute_t, void *));

extern int message_get_num_parts     __P ((message_t, size_t *nparts));
extern int message_set_get_num_parts __P ((message_t, int (*_get_num_parts)
					   __P ((message_t, size_t *)),
					   void *owner));

extern int message_get_part          __P ((message_t, size_t, message_t *));
extern int message_set_get_part      __P ((message_t, int (*_get_part)
					   __P ((message_t, size_t,
						 message_t *)), void *owner));

extern int message_get_uidl          __P ((message_t, char *, size_t, size_t *));
extern int message_set_uidl          __P ((message_t, int (*_get_uidl)
					   __P ((message_t, char *, size_t,
						 size_t *)), void *owner));

/* events */
#define MU_EVT_MSG_DESTROY 32
extern int message_register __P ((message_t msg, size_t type, int (*action)
				  __P ((size_t typ, void *arg)), void *arg));
extern int message_deregister __P ((message_t msg, void *action));

/* misc functions */
extern int message_create_attachment __P ((const char *content_type,
					   const char *encoding,
					   const char *filename,
					   message_t *newmsg));
extern int message_save_attachment __P ((message_t msg,
					 const char *filename, void **data));
extern int message_encapsulate __P ((message_t msg, message_t *newmsg,
				     void **data));
extern int message_unencapsulate __P ((message_t msg, message_t *newmsg,
				       void **data));

#ifdef _cplusplus
}
#endif

#endif /* _MESSAGE_H */
