/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2001, 2007, 2010-2012 Free Software Foundation,
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
   Public License along with this library.  If not, see 
   <http://www.gnu.org/licenses/>. */

#ifndef _MAILUTILS_SYS_BODY_H
# define _MAILUTILS_SYS_BODY_H

#include <mailutils/stream.h>
#include <mailutils/body.h>

#include <stdio.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _mu_body
{
  void *owner;
  mu_stream_t stream;
  mu_stream_t rawstream;
  mu_stream_t fstream;
  int flags;

  int (*_size)  (mu_body_t, size_t*);
  int (*_lines) (mu_body_t, size_t*);
  int (*_get_stream) (mu_body_t, mu_stream_t *);
};

#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_SYS_BODY_H */
