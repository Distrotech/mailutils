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

#ifndef MAILUTILS_SYS_TCP_H
#define MAILUTILS_SYS_TCP_H

#include <mailutils/refcount.h>
#include <mailutils/sys/stream.h>

enum _tcp_state
{
  TCP_STATE_INIT,
  TCP_STATE_RESOLVE,
  TCP_STATE_RESOLVING,
  TCP_STATE_CONNECTING,
  TCP_STATE_CONNECTED
};

struct _tcp_instance
{
  struct _stream base;
  mu_refcount_t refcount;
  int  fd;
  char *host;
  int  port;
  int  state;
  int flags;
  unsigned long address;
};

#endif /* _MAILUTILS_SYS_TCP_H */
