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

#ifndef _MAILUTILS_TYPES_H
#define _MAILUTILS_TYPES_H

#include <sys/types.h>

#ifndef __P
# if defined PROTOTYPES || (defined __STDC__ && __STDC__)
#  define __P(Args) Args
# else
#  define __P(Args) ()
# endif
#endif

#ifndef __PMT
# if defined PROTOTYPES || (defined __STDC__ && STDC__)
#  define __PMT(Args) Args
# else
#  define __PMT(Args) ()
# endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct _address;
struct _attribute;
struct _authority;
struct _body;
struct _debug;
struct _envelope;
struct _filter;
struct _filter_record;
struct _folder;
struct _header;
struct _iterator;
struct _list;
struct _locker;
struct _mailbox;
struct _mailer;
struct _message;
struct _mime;
struct _observable;
struct _observer;
struct _property;
struct _record;
struct _stream;
struct _ticket;
struct _url;
struct _wicket;

typedef struct _address *address_t;
typedef struct _attribute *attribute_t;
typedef struct _authority *authority_t;
typedef struct _body *body_t;
typedef struct _debug *mu_debug_t;
typedef struct _envelope *envelope_t;
typedef struct _filter *filter_t;
typedef struct _filter_record *filter_record_t;
typedef struct _folder *folder_t;
typedef struct _header * header_t;
typedef struct _iterator *iterator_t;
typedef struct _list *list_t;
typedef struct _locker *locker_t;
typedef struct _mailbox *mailbox_t;
typedef struct _mailer *mailer_t;
typedef struct _message *message_t;
typedef struct _mime *mime_t;
typedef struct _observable *observable_t;
typedef struct _observer *observer_t;
typedef struct _property *property_t;
typedef struct _record* record_t;
typedef struct _stream *stream_t;
typedef struct _ticket *ticket_t;
typedef struct _url *url_t;
typedef struct _wicket *wicket_t;

#ifdef __cplusplus
}
#endif

#endif

