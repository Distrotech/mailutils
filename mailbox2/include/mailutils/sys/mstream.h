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

#ifndef MAILUTILS_SYS_MSTREAM_H
#define MAILUTILS_SYS_MSTREAM_H

#include <mailutils/sys/stream.h>
#include <mailutils/refcount.h>

struct _ms
{
  struct _stream base;
  mu_refcount_t refcount;
  int fd;
  int flags;
  int mflags;
  char *ptr;
  size_t size;
  off_t offset;
};

#endif /* _MAILUTILS_SYS_MSTREAM_H */
