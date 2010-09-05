/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2009 Free Software Foundation, Inc.

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

#define _MU_STR_DIRTY         0x1000    /* Buffer dirty */
#define _MU_STR_WRT           0x2000    /* Unflushed write pending */
#define _MU_STR_ERR           0x4000    /* Permanent error state */
#define _MU_STR_EOF           0x8000    /* EOF encountered */

#define _MU_STR_INTERN_MASK   0xf000

#define _MU_STR_EVENT_SET     1
#define _MU_STR_EVENT_CLR     2

struct _mu_stream
{
  int ref_count;
  
  enum mu_buffer_type buftype;
  size_t bufsize;
  char *buffer;
  size_t level;
  char *cur;

  int flags;
  mu_off_t offset;
  mu_off_t bytes_in, bytes_out;
  
  int last_err;

  int (*read) (struct _mu_stream *, char *, size_t, size_t *);
  int (*readdelim) (struct _mu_stream *, char *, size_t, int, size_t *);
  int (*write) (struct _mu_stream *, const char *, size_t, size_t *);
  int (*flush) (struct _mu_stream *);
  int (*open) (struct _mu_stream *);
  int (*close) (struct _mu_stream *);
  void (*done) (struct _mu_stream *);
  int (*seek) (struct _mu_stream *, mu_off_t, mu_off_t *);
  int (*size) (struct _mu_stream *, mu_off_t *);
  int (*ctl) (struct _mu_stream *, int, void *);
  int (*wait) (struct _mu_stream *, int *, struct timeval *);
  int (*truncate) (struct _mu_stream *, mu_off_t);
  int (*shutdown) (struct _mu_stream *, int);

  void (*event_cb) (struct _mu_stream *, int, int);
  int  event_mask;
  
  const char *(*error_string) (struct _mu_stream *, int);
  
};

mu_stream_t _mu_stream_create (size_t size, int flags);
int mu_stream_read_unbuffered (mu_stream_t stream, void *buf, size_t size,
			       int full_read, size_t *pnread);
int mu_stream_write_unbuffered (mu_stream_t stream,
				const void *buf, size_t size,
				int full_write, size_t *pnwritten);

void _mu_stream_cleareof (mu_stream_t str);
void _mu_stream_seteof (mu_stream_t str);

#define _MU_SWAP_FIRST_ONLY         0x01
#define _MU_SWAP_IOCTL_MUST_SUCCEED 0x02

int _mu_stream_swap_streams (mu_stream_t stream, mu_stream_t *curtrans,
			     mu_stream_t *newtrans, int flags);


#endif
