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
# define _MAILUTILS_STREAM_H

#include <stdio.h>
#include <sys/types.h>

#include <mailutils/property.h>

#ifdef __cplusplus
extern "C" { /*}*/
#endif

#ifndef __P
# ifdef __STDC__
#  define __P(args) args
# else
#  define __P(args) ()
# endif
#endif /*__P */

struct _stream;
typedef struct _stream *stream_t;

#define MU_STREAM_READ	   0x00000001
#define MU_STREAM_WRITE	   0x00000002
#define MU_STREAM_RDWR     0x00000004
#define MU_STREAM_APPEND   0x00000008
#define MU_STREAM_CREAT	   0x00000010
#define MU_STREAM_NONBLOCK 0x00000020
/* Stream will be destroy on stream_destroy without checking the owner. */
#define MU_STREAM_NO_CHECK 0x00000040
#define MU_STREAM_SEEKABLE 0x00000080
#define MU_STREAM_NO_CLOSE 0x00000100

/* Functions useful to users of the pre-defined stream types. */

extern int file_stream_create    __P ((stream_t *stream, const char* filename, int flags));
extern int tcp_stream_create     __P ((stream_t *stream, const char* host, int port, int flags));
extern int mapfile_stream_create __P ((stream_t *stream, const char* filename, int flags));
extern int memory_stream_create  __P ((stream_t *stream, const char* filename, int flags));
extern int encoder_stream_create __P ((stream_t *stream, stream_t iostream, const char *encoding));
extern int decoder_stream_create __P ((stream_t *stream, stream_t iostream, const char *encoding));

extern void stream_destroy    __P ((stream_t *, void *owner));

extern int stream_open        __P ((stream_t));
extern int stream_close       __P ((stream_t));
extern int stream_is_seekable __P ((stream_t));
extern int stream_get_fd      __P ((stream_t , int *));
extern int stream_read        __P ((stream_t, char *, size_t, off_t, size_t *));
extern int stream_readline    __P ((stream_t, char *, size_t, off_t, size_t *));
extern int stream_size        __P ((stream_t, off_t *));
extern int stream_truncate    __P ((stream_t, off_t));
extern int stream_write       __P ((stream_t, const char *, size_t, off_t, size_t *));
extern int stream_setbufsiz   __P ((stream_t stream, size_t size));
extern int stream_flush       __P ((stream_t));


/* Functions useful to implementors of new stream types. */

extern int stream_create       __P ((stream_t *stream, int flags, void* owner));

extern void* stream_get_owner  __P ((stream_t));
extern void stream_set_owner   __P ((stream_t, void* owner));

extern int stream_get_flags    __P ((stream_t, int *pflags));
extern int stream_set_flags    __P ((stream_t, int flags));

extern int stream_get_property __P ((stream_t, property_t *));
extern int stream_set_property __P ((stream_t, property_t, void *));

#define MU_STREAM_STATE_OPEN  1
#define MU_STREAM_STATE_READ  2
#define MU_STREAM_STATE_WRITE 4
#define MU_STREAM_STATE_CLOSE 8
extern int stream_get_state    __P ((stream_t, int *pstate));

extern int stream_set_destroy  __P ((stream_t,
      void (*_destroy) __P ((stream_t)), void *owner));

extern int stream_set_open     __P ((stream_t,
      int (*_open) __P ((stream_t)), void *owner));

extern int stream_set_close    __P ((stream_t,
      int (*_close) __P ((stream_t)), void *owner));

extern int stream_set_fd       __P ((stream_t,
      int (*_get_fd)(stream_t, int *), void *owner));

extern int stream_set_read     __P ((stream_t,
      int (*_read) __P ((stream_t, char *, size_t, off_t, size_t *)), void *owner));

extern int stream_set_readline __P ((stream_t,
      int (*_readline) __P ((stream_t, char *, size_t, off_t, size_t *)), void *owner));

extern int stream_set_size     __P ((stream_t,
      int (*_size) __P ((stream_t, off_t *)), void *owner));

extern int stream_set_truncate __P ((stream_t,
      int (*_truncate) __P ((stream_t, off_t)), void *owner));

extern int stream_set_write    __P ((stream_t,
      int (*_write) __P ((stream_t, const char *, size_t, off_t, size_t *)), void *owner));

extern int stream_set_flush    __P ((stream_t,
      int (*_flush) __P ((stream_t)), void *owner));

#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_STREAM_H */

