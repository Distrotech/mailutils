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

#ifndef _MAILUTILS_STREAM_H
# define _MAILUTILS_STREAM_H

#include <stdio.h>

#include <mailutils/types.h>

#ifdef __cplusplus
extern "C" { /*}*/
#endif

#define MU_STREAM_READ	      0x00000001
#define MU_STREAM_WRITE	      0x00000002
#define MU_STREAM_RDWR        0x00000004
#define MU_STREAM_APPEND      0x00000008
#define MU_STREAM_CREAT	      0x00000010
#define MU_STREAM_NONBLOCK    0x00000020
/* Stream will be destroyed on stream_destroy without checking the owner. */
#define MU_STREAM_NO_CHECK    0x00000040
#define MU_STREAM_SEEKABLE    0x00000080
#define MU_STREAM_NO_CLOSE    0x00000100
#define MU_STREAM_ALLOW_LINKS 0x00000200
#define MU_STREAM_STRICT      0x00000400
  
/* Functions useful to users of the pre-defined stream types. */

extern int file_stream_create    __P ((stream_t *stream, const char* filename,
				       int flags));
extern int tcp_stream_create     __P ((stream_t *stream, const char* host,
				       int port, int flags));
extern int mapfile_stream_create __P ((stream_t *stream, const char* filename,
				       int flags));
extern int memory_stream_create  __P ((stream_t *stream, const char* filename,
				       int flags));
extern int encoder_stream_create __P ((stream_t *stream, stream_t iostream,
				       const char *encoding));
extern int decoder_stream_create __P ((stream_t *stream, stream_t iostream,
				       const char *encoding));
extern int stdio_stream_create   __P ((stream_t *stream, FILE* stdio,
				       int flags));
extern int prog_stream_create    __P ((stream_t *stream, char *progname,
				       int flags));
int filter_prog_stream_create    __P ((stream_t *stream, char *progname,
				       stream_t input));
  
extern void stream_destroy    __P ((stream_t *, void *owner));

extern int stream_open        __P ((stream_t));
extern int stream_close       __P ((stream_t));
extern int stream_is_seekable __P ((stream_t));
extern int stream_get_transport2 __P ((stream_t stream,
				       mu_transport_t *pt,
				       mu_transport_t *pt2));
extern int stream_get_transport __P ((stream_t stream,
				      mu_transport_t *pt));

extern int stream_read        __P ((stream_t, char *, size_t, off_t,
				    size_t *));
extern int stream_readline    __P ((stream_t, char *, size_t, off_t,
				    size_t *));
extern int stream_size        __P ((stream_t, off_t *));
extern int stream_truncate    __P ((stream_t, off_t));
extern int stream_write       __P ((stream_t, const char *, size_t, off_t,
				    size_t *));
extern int stream_setbufsiz   __P ((stream_t stream, size_t size));
extern int stream_flush       __P ((stream_t));

#define MU_STREAM_READY_RD 0x1
#define MU_STREAM_READY_WR 0x2
struct timeval;  /* Needed for the following declaration */ 

extern int stream_wait        __P((stream_t stream, int *pflags,
				   struct timeval *));

/* Functions useful to implementors of new stream types. */

extern int stream_create       __P ((stream_t *stream, int flags,
				     void* owner));

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
      void (*_destroy) __PMT ((stream_t)), void *owner));

extern int stream_set_open     __P ((stream_t,
      int (*_open) __PMT ((stream_t)), void *owner));

extern int stream_set_close    __P ((stream_t,
      int (*_close) __PMT ((stream_t)), void *owner));

extern int stream_set_get_transport2  __P ((stream_t,
      int (*_get_fd) __PMT ((stream_t, mu_transport_t *, mu_transport_t *)),
      void *owner));

extern int stream_set_read     __P ((stream_t,
      int (*_read) __PMT ((stream_t, char *, size_t, off_t, size_t *)),
			   void *owner));

extern int stream_set_readline __P ((stream_t,
      int (*_readline) __PMT ((stream_t, char *, size_t, off_t, size_t *)),
			       void *owner));

extern int stream_set_size     __P ((stream_t,
      int (*_size) __PMT ((stream_t, off_t *)), void *owner));

extern int stream_set_truncate __P ((stream_t,
      int (*_truncate) __PMT ((stream_t, off_t)), void *owner));

extern int stream_set_write    __P ((stream_t,
      int (*_write) __PMT ((stream_t, const char *, size_t, off_t, size_t *)),
			    void *owner));

extern int stream_set_flush    __P ((stream_t,
      int (*_flush) __PMT ((stream_t)), void *owner));

extern int stream_set_strerror __P ((stream_t stream,
      int (*fp) (stream_t, char **), void *owner));

extern int stream_set_wait __P ((stream_t stream,
      int (*wait) (stream_t, int *, struct timeval *), void *owner));
  
extern int stream_sequential_read __P((stream_t stream,
      char *buf, size_t size, size_t *nbytes));
  
extern int stream_sequential_readline __P((stream_t stream,
      char *buf, size_t size, size_t *nbytes));
  
extern int stream_sequential_write __P((stream_t stream,
					char *buf, size_t size));
extern int stream_seek __P((stream_t stream, off_t off, int whence));
  
extern int stream_strerror __P((stream_t stream, char **p));
  
#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_STREAM_H */

