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

#ifndef _MAILUTILS_BASE_H
#define _MAILUTILS_BASE_H

#include <sys/types.h>

#ifdef  __cplusplus
# define __MAILUTILS_START_DECLS  extern "C" {
# define __MAILUTILS_END_DECLS    }
#else
# define __MAILUTILS_BEGIN_DECLS
# define __MAILUTILS_END_DECLS
#endif

#ifndef __P
# ifdef __STDC__
#  define __P(args) args
# else
#  define __P(args) ()
# endif
#endif /*__P */

__MAILUTILS_BEGIN_DECLS

/* Forward declarations.  */
struct _url;
typedef struct _url *url_t;

struct _mailer;
typedef struct _mailer *mailer_t;

struct _folder;
typedef struct _folder *folder_t;

struct _mailbox;
typedef struct _mailbox *mailbox_t;

struct _mime;
struct _mime *mime_t;

struct _message;
typedef struct _message *message_t;

struct _attribute;
typedef struct _attribute *attribute_t;

struct _envelope;
typedef struct _envelope *envelope_t;

struct _header;
typedef struct _header *header_t;

struct _body;
typedef struct _body *body_t;

struct _ticket;
typedef struct _ticket *ticket_t;

struct _authority;
typedef struct _authority *authority_t;

struct _observable;
typedef struct _observable *observable_t;

struct _locker;
typedef struct _locker *locker_t;

struct _address;
typedef struct _address *address_t;

struct _debug;
typedef struct _debug *debug_t;

struct _stream;
typedef struct _stream *stream_t;

struct _filter;
typedef struct _filter *filter_t;

struct _iterator;
typedef struct _iterator *iterator_t;

struct _property;
typedef struct _property *property_t;

struct _monitor;
typedef struct _monitor *monitory_t;

struct _list;
typedef struct _list *list_t;

__MAILUTILS_END_DECLS

#endif /*_MAILUTILS_BASE_H */
