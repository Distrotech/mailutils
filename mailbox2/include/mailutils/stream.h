/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Library Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifndef _MAILUTILS_STREAM_H
#define _MAILUTILS_STREAM_H

#include <stdio.h>
#include <mailutils/mu_features.h>
#include <mailutils/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MU_STREAM_READ	   0x00000001
#define MU_STREAM_WRITE	   0x00000002
#define MU_STREAM_RDWR     (MU_STREAM_READ|MU_STREAM_WRITE)
#define MU_STREAM_APPEND   0x00000004
#define MU_STREAM_CREAT	   0x00000008
#define MU_STREAM_NONBLOCK 0x00000010

enum stream_state
{
  MU_STREAM_NO_STATE,
  MU_STREAM_STATE_OPEN,
  MU_STREAM_STATE_READ,
  MU_STREAM_STATE_WRITE,
  MU_STREAM_STATE_CLOSE
};

extern int  stream_ref __P ((stream_t));
extern void stream_destroy __P ((stream_t *));

extern int  stream_open __P ((stream_t, const char *, int, int));
extern int  stream_close __P ((stream_t));

extern int  stream_read __P ((stream_t, void *, size_t, off_t, size_t *));
extern int  stream_readline __P ((stream_t, char *, size_t, off_t, size_t *));
extern int  stream_write __P ((stream_t, const void *, size_t, off_t, size_t*));

extern int  stream_tell  __P ((stream_t, off_t *));

extern int  stream_get_size __P ((stream_t, off_t *));
extern int  stream_truncate __P ((stream_t, off_t));
extern int  stream_flush __P ((stream_t));

extern int  stream_get_fd __P ((stream_t , int *));
extern int  stream_get_flags __P ((stream_t, int *));
extern int  stream_get_state __P ((stream_t, enum stream_state  *));

extern int  stream_is_seekable __P ((stream_t));
extern int  stream_is_readready __P ((stream_t, int));
extern int  stream_is_writeready __P ((stream_t, int));
extern int  stream_is_exceptionpending __P ((stream_t, int));

extern int  stream_is_open __P ((stream_t));

/* Misc.  */
extern int  stream_file_create    __P ((stream_t *));
extern int  stream_stdio_create   __P ((stream_t *, FILE *));
extern int  stream_mmap_create    __P ((stream_t *));
extern int  stream_memory_create  __P ((stream_t *, size_t));
extern int  stream_tcp_create     __P ((stream_t *));
extern int  stream_fd_create      __P ((stream_t *, int));
extern int  stream_buffer_create  __P ((stream_t *, stream_t, size_t));

#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_STREAM_H */
