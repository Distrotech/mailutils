/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000 Free Software Foundation, Inc.

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

#ifndef _MAILER0_H
#define _MAILER0_H

#ifdef DMALLOC
#  include <dmalloc.h>
#endif

#include <sys/types.h>
#include <mailutils/mailer.h>
#ifdef HAVE_PTHREAD_H
#  define __USE_UNIX98 /* ?? */
#  include <pthread.h>
#endif
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

// mailer states
#define MAILER_STATE_HDR		1
#define MAILER_STATE_MSG		2
#define MAILER_STATE_COMPLETE	3

// mailer messages
#define MAILER_HELO	1
#define MAILER_MAIL	2
#define MAILER_RCPT	3
#define MAILER_DATA	4
#define MAILER_RSET	5
#define MAILER_QUIT	6

#define MAILER_LINE_BUF_SIZE	1000

struct _mailer
{
  stream_t stream;
  observable_t observable;
  debug_t debug;
  url_t url;
  int flags;
#ifdef WITH_PTHREAD
  pthread_rwlock_t rwlock;
#endif

  /* Pointer to the specific mailer data.  */
  void *data;

  /* Public methods.  */
  int (*_init)         __P ((mailer_t));
  void (*_destroy)     __P ((mailer_t));
  int (*_open)         __P ((mailer_t, int flags));
  int (*_close)        __P ((mailer_t));
  int (*_send_message) __P ((mailer_t, const char *from, const char *rcpt,
			     int dsn, message_t));
};

/* Mail locks.  */
extern int mailer_rdlock __P ((mailer_t));
extern int mailer_wrlock __P ((mailer_t));
extern int mailer_unlock __P ((mailer_t));

#define MAILER_NOTIFY(mailer, type) \
if (mailer->observer) observer_notify (mailer->observer, type)

/* Moro(?)ic kluge.  */
#define MAILER_DEBUG0(mailer, type, format) \
if (mailer->debug) debug_print (mailer->debug, type, format)
#define MAILER_DEBUG1(mailer, type, format, arg1) \
if (mailer->debug) debug_print (mailer->debug, type, format, arg1)
#define MAILER_DEBUG2(mailer, type, format, arg1, arg2) \
if (mailer->debug) debug_print (mailer->debug, type, format, arg1, arg2)
#define MAILER_DEBUG3(mailer, type, format, arg1, arg2, arg3) \
if (mailer->debug) debug_print (mailer->debug, type, format, arg1, arg2, arg3)
#define MAILER_DEBUG4(mailer, type, format, arg1, arg2, arg3, arg4) \
if (mailer->debug) debug_print (mailer->debug, type, format, arg1, arg2, arg3, arg4)

#ifdef _cplusplus
}
#endif

#endif /* MAILER0_H */
