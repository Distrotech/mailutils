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

#ifndef _MAILER0_H
#define _MAILER0_H

#ifdef DMALLOC
#  include <dmalloc.h>
#endif

#include <sys/types.h>
#include <mailutils/mailer.h>
#include <mailutils/monitor.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Default mailer URL. */

#define MAILER_URL_DEFAULT "sendmail:"

#define MAILER_LINE_BUF_SIZE	1000

struct _mailer
{
  stream_t stream;
  observable_t observable;
  mu_debug_t debug;
  url_t url;
  int flags;
  monitor_t monitor;
  property_t property;

  /* Pointer to the specific mailer data.  */
  void *data;

  /* Public methods.  */
  void (*_destroy)     __P ((mailer_t));
  int (*_open)         __P ((mailer_t, int flags));
  int (*_close)        __P ((mailer_t));
  int (*_send_message) __P ((mailer_t, message_t, address_t, address_t));
};

#define MAILER_NOTIFY(mailer, type) \
  if (mailer->observer) observer_notify (mailer->observer, type)

/* Moro(?)ic kluge.  */
#define MAILER_DEBUGV(mailer, type, format, av) \
  if (mailer->debug) mu_debug_print (mailer->debug, type, format, av)
#define MAILER_DEBUG0(mailer, type, format) \
  if (mailer->debug) mu_debug_print (mailer->debug, type, format)
#define MAILER_DEBUG1(mailer, type, format, arg1) \
  if (mailer->debug) mu_debug_print (mailer->debug, type, format, arg1)
#define MAILER_DEBUG2(mailer, type, format, arg1, arg2) \
  if (mailer->debug) mu_debug_print (mailer->debug, type, format, arg1, arg2)
#define MAILER_DEBUG3(mailer, type, format, arg1, arg2, arg3) \
  if (mailer->debug) mu_debug_print (mailer->debug, type, format, arg1, arg2, arg3)
#define MAILER_DEBUG4(mailer, type, format, arg1, arg2, arg3, arg4) \
  if (mailer->debug) mu_debug_print (mailer->debug, type, format, arg1, arg2, arg3, arg4)

#ifdef __cplusplus
}
#endif

#endif /* MAILER0_H */
