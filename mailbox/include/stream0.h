/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Library Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with GNU Mailutils; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifndef _STREAM0_H
#define _STREAM0_H

#include <mailutils/stream.h>
#ifdef DMALLOC
#include <dmalloc.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Read buffer */
struct rbuffer
{
  char *base;
  char *ptr;
  int  count;
  size_t bufsiz;
  off_t offset;
};

struct _stream
{
  void *owner;
  property_t property;

  int flags;
  int state;

  /* Read space.  */
  off_t roff;
  struct rbuffer rbuffer;

  void (*_destroy) __P ((stream_t));
  int (*_open)     __P ((stream_t));
  int (*_close)    __P ((stream_t));
  int (*_get_fd)   __P ((stream_t, int *));
  int (*_read)     __P ((stream_t, char *, size_t, off_t, size_t *));
  int (*_readline) __P ((stream_t, char *, size_t, off_t, size_t *));
  int (*_write)    __P ((stream_t, const char *, size_t, off_t, size_t *));
  int (*_truncate) __P ((stream_t, off_t));
  int (*_size)     __P ((stream_t, off_t *));
  int (*_flush)    __P ((stream_t));
  int (*_setbufsiz)__P ((stream_t, size_t));
};

#ifdef __cplusplus
}
#endif

#endif /* _STREAM0_H */
