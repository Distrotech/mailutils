/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2005, 2007, 2010 Free Software
   Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library; if not, write to the
   Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301 USA */

#ifndef _MAILUTILS_SYS_FILTER_H
#define _MAILUTILS_SYS_FILTER_H

#include <mailutils/filter.h>
#include <mailutils/sys/stream.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MU_FILTER_BUF_SIZE 2048

struct _mu_filter_buffer
{
  char *base;
  size_t size;
  size_t level;
  size_t pos;
};

struct _mu_filter_stream
{
  struct _mu_stream stream;
  mu_stream_t transport;
  int mode;
  
  struct _mu_filter_buffer inbuf, outbuf;
  mu_filter_xcode_t xcode;
  void *xdata;
};

#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_SYS_FILTER_H */