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

#include <mailutils/sys/stream.h>
#include <mailutils/monitor.h>

/* Read buffer */
struct _rbuffer
{
  char *base;
  char *ptr;
  int  count;
  size_t bufsize;
};

struct _bs
{
  struct _stream base;
  int ref;
  stream_t stream;
  struct _rbuffer rbuffer;
  monitor_t lock;
};

#endif /* _MAILUTILS_SYS_TCP_H */
