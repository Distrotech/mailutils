/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU Library General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifndef _MAILUTILS_MAILER_H
#define _MAILUTILS_MAILER_H

#include <sys/types.h>

#include <mailutils/mu_features.h>
#include <mailutils/address.h>
#include <mailutils/debug.h>
#include <mailutils/message.h>
#include <mailutils/observer.h>
#include <mailutils/property.h>
#include <mailutils/stream.h>
#include <mailutils/url.h>

#ifdef __cplusplus
extern "C" {
#endif

/* forward declaration */
struct _mailer;
typedef struct _mailer *mailer_t;

extern int mailer_create        __P ((mailer_t *, const char *));
extern void mailer_destroy      __P ((mailer_t *));

extern int mailer_open          __P ((mailer_t, int flags));
extern int mailer_close         __P ((mailer_t));

extern int mailer_send_message  __P ((mailer_t, message_t, address_t from, address_t to));

extern int mailer_get_property  __P ((mailer_t, property_t *));
/* stream settings */
extern int mailer_get_stream    __P ((mailer_t, stream_t *));
extern int mailer_set_stream    __P ((mailer_t, stream_t));

/* stream settings */
extern int mailer_get_debug     __P ((mailer_t, mu_debug_t *));
extern int mailer_set_debug     __P ((mailer_t, mu_debug_t));

extern int mailer_get_observable __P ((mailer_t, observable_t *));

extern int mailer_get_url       __P ((mailer_t, url_t *));

#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_MAILER_H */
