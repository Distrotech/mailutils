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

#ifndef MAILUTILS_SYS_FDSTREAM_H
#define MAILUTILS_SYS_FDSTREAM_H

#include <mailutils/refcount.h>
#include <mailutils/sys/stream.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _stream_fd
{
  struct _stream base;
  mu_refcount_t refcount;
  int  fd;
  int  state;
  int flags;
};

int  _stream_fd_ctor __P ((struct _stream_fd *, int));
void _stream_fd_dtor __P ((struct _stream_fd *));

int  _stream_fd_ref  __P ((stream_t));
void _stream_fd_destroy __P ((stream_t *));
int  _stream_fd_open    __P ((stream_t, const char *, int, int));
int  _stream_fd_close   __P ((stream_t));
int  _stream_fd_read    __P ((stream_t, void *, size_t, size_t *));
int  _stream_fd_readline __P ((stream_t, char *, size_t, size_t *));
int  _stream_fd_write    __P ((stream_t, const void *, size_t, size_t *));
int  _stream_fd_get_fd   __P ((stream_t, int *));
int  _stream_fd_get_flags __P ((stream_t, int *));
int  _stream_fd_get_size  __P ((stream_t, off_t *));
int  _stream_fd_truncate  __P ((stream_t, off_t));
int  _stream_fd_flush     __P ((stream_t));
int  _stream_fd_get_state __P ((stream_t, enum stream_state *));
int  _stream_fd_seek      __P ((stream_t, off_t, enum stream_whence));
int  _stream_fd_tell      __P ((stream_t, off_t *));
int  _stream_fd_is_readready __P ((stream_t, int ));
int  _stream_fd_is_writeready __P ((stream_t, int));
int  _stream_fd_is_exceptionpending __P ((stream_t, int));
int  _stream_fd_is_open __P ((stream_t));

#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_SYS_FDSTREAM_H */
