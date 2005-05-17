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

   You should have received a copy of the GNU Lesser General
   Public License along with this library; if not, write to the
   Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301 USA */

#ifndef _FOLDER0_H
#define _FOLDER0_H

#ifdef DMALLOC
#  include <dmalloc.h>
#endif

#include <sys/types.h>
#include <stdio.h>

#include <mailutils/monitor.h>
#include <mailutils/folder.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _folder
{
  /* Data */
  authority_t authority;
  observable_t observable;
  mu_debug_t debug;
  stream_t stream;
  monitor_t monitor;
  url_t url;
  int flags;
  int ref;
  size_t uid;

  /* Back pointer to the specific mailbox */
  void *data;

  /* Public methods */

  void (*_destroy)     __P ((folder_t));

  int  (*_open)        __P ((folder_t, int flag));
  int  (*_close)       __P ((folder_t));
  int  (*_list)        __P ((folder_t, const char *, const char *,
			     struct folder_list *));
  int  (*_lsub)        __P ((folder_t, const char *, const char *,
			     struct folder_list *));
  int  (*_delete)      __P ((folder_t, const char *));
  int  (*_rename)      __P ((folder_t, const char *, const char *));
  int  (*_subscribe)   __P ((folder_t, const char *));
  int  (*_unsubscribe) __P ((folder_t, const char *));
};

/* Moro(?)ic kluge.  */
#define FOLDER_DEBUG0(folder, type, format) \
if (folder->debug) mu_debug_print (folder->debug, type, format)
#define FOLDER_DEBUG1(folder, type, format, arg1) \
if (folder->debug) mu_debug_print (folder->debug, type, format, arg1)
#define FOLDER_DEBUG2(folder, type, format, arg1, arg2) \
if (folder->debug) mu_debug_print (folder->debug, type, format, arg1, arg2)
#define FOLDER_DEBUG3(folder, type, format, arg1, arg2, arg3) \
if (folder->debug) mu_debug_print (folder->debug, type, format, arg1, arg2, arg3)
#define FOLDER_DEBUG4(folder, type, format, arg1, arg2, arg3, arg4) \
if (folder->debug) mu_debug_print (folder->debug, type, format, arg1, arg2, arg3, arg4)

#ifdef __cplusplus
}
#endif

#endif /* _FOLDER0_H */
