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

#ifndef MAILUTILS_SYS_TCP_H
#define MAILUTILS_SYS_TCP_H

#ifdef DMALLOC
# include <dmalloc.h>
#endif

#include <mailutils/refcount.h>
#include <mailutils/sys/fdstream.h>

#ifdef __cplusplus
extern "C" {
#endif

enum _stream_tcp_state
{
  TCP_STATE_INIT,
  TCP_STATE_RESOLVE,
  TCP_STATE_RESOLVING,
  TCP_STATE_CONNECTING,
  TCP_STATE_CONNECTED
};

struct _stream_tcp
{
  struct _stream_fd base;
  char *host;
  int  port;
  int  state;
  unsigned long address;
};

void _stream_tcp_destroy __P ((stream_t *));
int  _stream_tcp_close __P ((stream_t));
int _stream_tcp_open     __P ((stream_t, const char *, int, int));

#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_SYS_TCP_H */
