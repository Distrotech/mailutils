/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2005  Free Software Foundation, Inc.

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
  size_t count;
  size_t bufsiz;
  off_t offset;
};

struct _stream
{
  void *owner;
  property_t property;

  int flags;
  int state;

  /* Read space */
  struct rbuffer rbuffer;
  
  /* Stream pointer for sequential offset.  */
  off_t offset;
  
  void (*_destroy) (stream_t);
  int (*_open)     (stream_t);
  int (*_close)    (stream_t);
  int (*_get_transport2) (stream_t, mu_transport_t *, mu_transport_t *);
  int (*_read)     (stream_t, char *, size_t, off_t, size_t *);
  int (*_readline) (stream_t, char *, size_t, off_t, size_t *);
  int (*_write)    (stream_t, const char *, size_t, off_t, size_t *);
  int (*_truncate) (stream_t, off_t);
  int (*_size)     (stream_t, off_t *);
  int (*_flush)    (stream_t);
  int (*_setbufsiz)(stream_t, size_t);
  int (*_strerror) (stream_t, const char **);
  int (*_wait)     (stream_t, int *pflags, struct timeval *tvp);
};

#ifdef __cplusplus
}
#endif

#endif /* _STREAM0_H */
