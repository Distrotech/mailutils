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

#ifndef MAILUTILS_SYS_MEMSTREAM_H
#define MAILUTILS_SYS_MEMSTREAM_H

#include <sys/types.h>

#ifdef DMALLOC
# include <dmalloc.h>
#endif

#include <mailutils/refcount.h>
#include <mailutils/sys/stream.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MU_STREAM_MEMORY_BLOCKSIZE
#define MU_STREAM_MEMORY_BLOCKSIZE 512
#endif

struct _stream_memory
{
  struct _stream base;
  mu_refcount_t refcount;
  char *ptr;
  size_t capacity;
  size_t size;
  int flags;
};

extern int  _stream_memory_ctor __P ((struct _stream_memory *, size_t));
extern void _stream_memory_dtor __P ((stream_t));
extern int  _stream_memory_ref  __P ((stream_t));
extern void _stream_memory_destroy __P ((stream_t *));
extern int  _stream_memory_open    __P ((stream_t, const char *, int, int));
extern int  _stream_memory_close   __P ((stream_t));
extern int  _stream_memory_read __P ((stream_t, void *, size_t, off_t, size_t *));
extern int  _stream_memory_readline __P ((stream_t, char *, size_t, off_t, size_t *));
extern int  _stream_memory_write __P ((stream_t, const void *, size_t, off_t, size_t *));
extern int  _stream_memory_get_fd __P ((stream_t, int *));
extern int  _stream_memory_get_flags __P ((stream_t, int *));
extern int  _stream_memory_get_size  __P ((stream_t, off_t *));
extern int  _stream_memory_truncate  __P ((stream_t, off_t));
extern int  _stream_memory_flush     __P ((stream_t));
extern int  _stream_memory_get_state __P ((stream_t, enum stream_state *));
extern int  _stream_memory_tell __P ((stream_t, off_t *));
extern int  _stream_memory_is_seekable __P ((stream_t));
extern int  _stream_memory_is_readready __P ((stream_t, int ));
extern int  _stream_memory_is_writeready __P ((stream_t, int));
extern int  _stream_memory_is_exceptionpending __P ((stream_t, int));
extern int  _stream_memory_is_open __P ((stream_t));

#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_SYS_MEMSTREAM_H */
