/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2005  Free Software Foundation, Inc.

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

#ifndef _MAILUTILS_MAILER_H
#define _MAILUTILS_MAILER_H

#include <mailutils/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* During protocol traces, the data as well as headers will be dumped. */
#define MAILER_FLAG_DEBUG_DATA 0x0001

/* A url of NULL will cause the default to be used. */
extern int mailer_create         (mailer_t *, const char *url);
extern void mailer_destroy       (mailer_t *);
extern int mailer_open           (mailer_t, int flags);
extern int mailer_close          (mailer_t);
extern int mailer_send_message   (mailer_t, message_t, address_t from, address_t to);

/* Called to set or get the default mailer url. */
extern int mailer_set_url_default       (const char* url);
extern int mailer_get_url_default       (const char** url);

/* Accessor functions. */
extern int mailer_get_property   (mailer_t, property_t *);
extern int mailer_get_stream     (mailer_t, stream_t *);
extern int mailer_set_stream     (mailer_t, stream_t);
extern int mailer_get_debug      (mailer_t, mu_debug_t *);
extern int mailer_set_debug      (mailer_t, mu_debug_t);
extern int mailer_get_observable (mailer_t, observable_t *);
extern int mailer_get_url        (mailer_t, url_t *);

/* Utility functions, primarily for use of implementing concrete mailers. */

/* A valid from address_t contains a single address that has a qualified
   email address. */
extern int mailer_check_from     (address_t from);
/* A valid to address_t contains 1 or more addresses, that are
   qualified email addresses. */
extern int mailer_check_to       (address_t to);

#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_MAILER_H */
