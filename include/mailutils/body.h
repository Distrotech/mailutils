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

   You should have received a copy of the GNU Lesser General
   Public License along with this library; if not, write to the
   Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301 USA */

#ifndef _MAILUTILS_BODY_H
#define _MAILUTILS_BODY_H

#include <mailutils/types.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int body_create         __P ((body_t *, void *owner));
extern void body_destroy       __P ((body_t *, void *owner));
extern void * body_get_owner   __P ((body_t));
extern int body_is_modified    __P ((body_t));
extern int body_clear_modified __P ((body_t));

extern int body_get_stream     __P ((body_t, stream_t *));
extern int body_set_stream     __P ((body_t, stream_t, void *owner));

extern int body_get_filename   __P ((body_t, char *, size_t, size_t *));

extern int body_size           __P ((body_t, size_t*));
extern int body_set_size       __P ((body_t, int (*_size)
				     __PMT ((body_t, size_t*)), void *owner));
extern int body_lines          __P ((body_t, size_t *));
extern int body_set_lines      __P ((body_t, int (*_lines)
				     __PMT ((body_t, size_t*)), void *owner));

#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_BODY_H */
