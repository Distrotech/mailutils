/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2005, 2007, 2010 Free Software Foundation,
   Inc.

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

#ifndef _MAILUTILS_SYS_DEBUG_H
# define _MAILUTILS_SYS_DEBUG_H

#include <mailutils/debug.h>
#include <mailutils/stream.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _mu_debug
{
  size_t level;
  mu_stream_t stream;
  void *owner;
  struct mu_debug_locus locus;
  const char *function;
  void *data;
  mu_debug_printer_fp printer;
  void (*destroy) (void *data);
};

#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_SYS_DEBUG_H */