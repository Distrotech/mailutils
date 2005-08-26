/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2005 Free Software Foundation, Inc.

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

extern int mu_body_create         (body_t *, void *owner);
extern void mu_body_destroy       (body_t *, void *owner);
extern void * mu_body_get_owner   (body_t);
extern int mu_body_is_modified    (body_t);
extern int mu_body_clear_modified (body_t);

extern int mu_body_get_stream     (body_t, stream_t *);
extern int mu_body_set_stream     (body_t, stream_t, void *owner);

extern int mu_body_get_filename   (body_t, char *, size_t, size_t *);

extern int mu_body_size           (body_t, size_t*);
extern int mu_body_set_size       (body_t,
				int (*_size) (body_t, size_t*), void *owner);
extern int mu_body_lines          (body_t, size_t *);
extern int mu_body_set_lines      (body_t,
				int (*_lines) (body_t, size_t*), void *owner);

#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_BODY_H */
