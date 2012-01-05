/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2009, 2011-2012 Free Software Foundation, Inc.

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

#ifndef _MAILUTILS_SYS_FILE_STREAM_H
#define _MAILUTILS_SYS_FILE_STREAM_H

#include <mailutils/types.h>
#include <mailutils/stream.h>
#include <mailutils/sys/stream.h>

#define _MU_FILE_STREAM_TEMP        0x01
#define _MU_FILE_STREAM_ECHO_OFF    0x02
#define _MU_FILE_STREAM_FD_BORROWED 0x04
#define _MU_FILE_STREAM_STATIC_FILENAME 0x08

struct _mu_file_stream
{
  struct _mu_stream stream;
  int fd;
  int flags;
  char *filename;
  void *echo_state;
};

int _mu_file_stream_create (struct _mu_file_stream **pstream, size_t size,
			    const char *filename, int fd, int flags);
void _mu_file_stream_setup (struct _mu_file_stream *str);

#endif
