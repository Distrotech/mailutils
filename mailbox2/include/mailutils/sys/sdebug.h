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

#ifndef _MAILUTILS_SYS_SDEBUG_H
#define _MAILUTILS_SYS_SDEBUG_H

#ifdef DMALLOC
#include <dmalloc.h>
#endif

#include <mailutils/refcount.h>
#include <mailutils/sys/debug.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _mu_debug_stream
{
  struct _mu_debug base;
  mu_refcount_t refcount;
  int level;
  off_t offset;
  stream_t stream;
  int close_on_destroy;
};

int  _mu_debug_stream_ctor __P ((struct _mu_debug_stream *));
int  _mu_debug_stream_dtor __P ((mu_debug_t));
int  _mu_debug_stream_ref  __P ((mu_debug_t));
void _mu_debug_stream_destroy __P ((mu_debug_t *));
int  _mu_debug_stream_set_level __P ((mu_debug_t, size_t));
int  _mu_debug_stream_get_level __P ((mu_debug_t, size_t *));
int  _mu_debug_stream_print     __P ((mu_debug_t, size_t, const char *));

#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_SYS_SDEBUG_H */
