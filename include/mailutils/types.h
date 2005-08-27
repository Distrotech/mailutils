/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2005 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library; if not, write to the
   Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301 USA */

#ifndef _MAILUTILS_TYPES_H
#define _MAILUTILS_TYPES_H

#include <sys/types.h>

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
struct mu_filter_record;
struct _folder;
struct _header;
struct _iterator;
struct _list;
struct _locker;
struct _mailbox;
struct _mailer;
struct _message;
struct _mime;
struct _mu_mailcap;
struct _mu_mailcap_entry;
struct _mu_refcount;
struct _observable;
struct _observer;
struct _property;
struct mu__record;
struct _stream;
struct _ticket;
struct _url;
struct _wicket;

typedef struct _address *mu_address_t;
typedef struct _attribute *mu_attribute_t;
typedef struct _authority *mu_authority_t;
typedef struct _body *mu_body_t;
typedef struct _debug *mu_debug_t;
typedef struct _envelope *mu_envelope_t;
typedef struct _filter *mu_filter_t;
typedef struct mu_filter_record *mu_filter_record_t;
typedef struct _folder *mu_folder_t;
typedef struct _header * mu_header_t;
typedef struct _iterator *mu_iterator_t;
typedef struct _list *mu_list_t;
typedef struct _locker *mu_locker_t;
typedef struct _mailbox *mu_mailbox_t;
typedef struct _mailer *mu_mailer_t;
typedef struct _message *mu_message_t;
typedef struct _mime *mu_mime_t;
typedef struct _mu_mailcap *mu_mailcap_t;
typedef struct _mu_mailcap_entry *mu_mailcap_entry_t;
typedef struct _mu_refcount *mu_refcount_t;
typedef struct _observable *mu_observable_t;
typedef struct _observer *mu_observer_t;
typedef struct _property *mu_property_t;
typedef struct mu__record *mu_record_t;
typedef struct _stream *mu_stream_t;
typedef struct _ticket *mu_ticket_t;
typedef struct _url *mu_url_t;
typedef struct _wicket *mu_wicket_t;
typedef void *mu_transport_t;

#define MU_FOLDER_ATTRIBUTE_DIRECTORY 0x001
#define MU_FOLDER_ATTRIBUTE_FILE      0x002
#define MU_FOLDER_ATTRIBUTE_ALL \
          (MU_FOLDER_ATTRIBUTE_DIRECTORY|MU_FOLDER_ATTRIBUTE_FILE)
  
#ifdef __cplusplus
}
#endif

#endif

