/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU Library General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifndef _MAILBOX0_H
#define _MAILBOX0_H

#ifdef DMALLOC
#  include <dmalloc.h>
#endif

#include <sys/types.h>
#include <stdio.h>

#include <mailutils/monitor.h>
#include <mailutils/mailbox.h>
#include <mailutils/folder.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __P
# ifdef __STDC__
#  define __P(args) args
# else
#  define __P(args) ()
# endif
#endif /*__P */

struct _mailbox
{
  /* Data */
  observable_t observable;
  debug_t debug;
  ticket_t ticket;
  authority_t authority;
  property_t property;
  locker_t locker;
  stream_t stream;
  url_t url;
  int flags;
  folder_t folder;
  monitor_t monitor;

  /* Back pointer to the specific mailbox */
  void *data;

  /* Public methods */

  void (*_destroy)         __P ((mailbox_t));

  int  (*_open)            __P ((mailbox_t, int flag));
  int  (*_close)           __P ((mailbox_t));

  /* messages */
  int  (*_get_message)     __P ((mailbox_t, size_t msgno, message_t *msg));
  int  (*_append_message)  __P ((mailbox_t, message_t msg));
  int  (*_messages_count)  __P ((mailbox_t, size_t *num));
  int  (*_messages_recent) __P ((mailbox_t, size_t *num));
  int  (*_message_unseen)  __P ((mailbox_t, size_t *num));
  int  (*_expunge)         __P ((mailbox_t));
  int  (*_uidvalidity)     __P ((mailbox_t, unsigned long *num));
  int  (*_uidnext)         __P ((mailbox_t, size_t *num));
  int  (*_get_property)    __P ((mailbox_t, property_t *num));

  int  (*_scan)            __P ((mailbox_t, size_t msgno, size_t *count));
  int  (*_is_updated)      __P ((mailbox_t));

  int  (*_size)            __P ((mailbox_t, off_t *size));

};

#define MAILBOX_NOTIFY(mbox, type) \
if (mbox->observer) observer_notify (mbox->observer, type)

/* Moro(?)ic kluge.  */
#define MAILBOX_DEBUG0(mbox, type, format) \
if (mbox->debug) debug_print (mbox->debug, type, format)
#define MAILBOX_DEBUG1(mbox, type, format, arg1) \
if (mbox->debug) debug_print (mbox->debug, type, format, arg1)
#define MAILBOX_DEBUG2(mbox, type, format, arg1, arg2) \
if (mbox->debug) debug_print (mbox->debug, type, format, arg1, arg2)
#define MAILBOX_DEBUG3(mbox, type, format, arg1, arg2, arg3) \
if (mbox->debug) debug_print (mbox->debug, type, format, arg1, arg2, arg3)
#define MAILBOX_DEBUG4(mbox, type, format, arg1, arg2, arg3, arg4) \
if (mbox->debug) debug_print (mbox->debug, type, format, arg1, arg2, arg3, arg4)

#ifdef __cplusplus
}
#endif

#endif /* _MAILBOX0_H */
