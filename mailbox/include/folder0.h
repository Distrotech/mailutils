/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000 Free Software Foundation, Inc.

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

#ifndef _FOLDER_H
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

#ifndef __P
# ifdef __STDC__
#  define __P(args) args
# else
#  define __P(args) ()
# endif
#endif /*__P */

struct _folder
{
  /* Data */
  ticket_t ticket;
  authority_t authority;
  observable_t observable;
  debug_t debug;
  stream_t stream;
  monitor_t monitor;
  url_t url;
  int flags;
  int ref;
  size_t uid;

  /* Back pointer to the specific mailbox */
  void *data;

  /* Public methods */

  int  (*_init)            __P ((folder_t));
  void (*_destroy)         __P ((folder_t));

  int  (*_open)            __P ((folder_t, int flag));
  int  (*_close)           __P ((folder_t));
  int  (*_list)            __P ((folder_t, const char *,
				 struct folder_list ***, size_t *));
  int  (*_delete_mailbox)  __P ((folder_t, const char *));
};


#ifdef __cplusplus
}
#endif

#endif /* _MAILBOX0_H */
