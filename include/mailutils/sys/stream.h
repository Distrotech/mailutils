/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2009-2012 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>. */

#ifndef _MAILUTILS_SYS_STREAM_H
#define _MAILUTILS_SYS_STREAM_H

#define _MU_STR_OPEN          0x01000000    /* Stream open */
#define _MU_STR_DIRTY         0x02000000    /* Buffer dirty */
#define _MU_STR_WRT           0x04000000    /* Unflushed write pending */
#define _MU_STR_ERR           0x08000000    /* Permanent error state */
#define _MU_STR_EOF           0x10000000    /* EOF encountered */

#define _MU_STR_INTERN_MASK   0xff000000

#define _MU_STR_EVENT_SETFLAG   0
#define _MU_STR_EVENT_CLRFLAG   1
#define _MU_STR_EVENT_FILLBUF   2
#define _MU_STR_EVENT_FLUSHBUF  3
#define _MU_STR_EVENT_CLOSE     4
#define _MU_STR_EVENT_BOOTSTRAP 5

#define _MU_STR_EVMASK(n) (1<<(n))

struct _mu_stream
{
  int ref_count;
  
  enum mu_buffer_type buftype;
  size_t bufsize;
  char *buffer;
  size_t level;
  size_t pos;

  int flags;
  mu_off_t offset;
  
  int statmask;
  mu_off_t *statbuf;
  
  int last_err;

  int (*read) (struct _mu_stream *, char *, size_t, size_t *);
  int (*readdelim) (struct _mu_stream *, char *, size_t, int, size_t *);
  int (*write) (struct _mu_stream *, const char *, size_t, size_t *);
  int (*flush) (struct _mu_stream *);
  int (*open) (struct _mu_stream *);
  int (*close) (struct _mu_stream *);
  void (*done) (struct _mu_stream *);
  void (*destroy) (struct _mu_stream *);
  int (*seek) (struct _mu_stream *, mu_off_t, mu_off_t *);
  int (*size) (struct _mu_stream *, mu_off_t *);
  int (*ctl) (struct _mu_stream *, int, int, void *);
  int (*wait) (struct _mu_stream *, int *, struct timeval *);
  int (*truncate) (struct _mu_stream *, mu_off_t);
  int (*shutdown) (struct _mu_stream *, int);

  void (*event_cb) (struct _mu_stream *, int code, unsigned long, void *);
  int  event_mask;
  void *event_cb_data;

  int (*setbuf_hook) (mu_stream_t, enum mu_buffer_type, size_t);
  
  const char *(*error_string) (struct _mu_stream *, int);
  
};

mu_stream_t _mu_stream_create (size_t size, int flags);

void _mu_stream_cleareof (mu_stream_t str);
void _mu_stream_seteof (mu_stream_t str);

#endif
