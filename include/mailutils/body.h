/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2001, 2005, 2007, 2010-2012 Free Software
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
   Public License along with this library.  If not, see 
   <http://www.gnu.org/licenses/>. */

#ifndef _MAILUTILS_BODY_H
#define _MAILUTILS_BODY_H

#include <mailutils/types.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int mu_body_create         (mu_body_t *, void *owner);
extern void mu_body_destroy       (mu_body_t *, void *owner);
extern void * mu_body_get_owner   (mu_body_t);
extern int mu_body_is_modified    (mu_body_t);
extern int mu_body_clear_modified (mu_body_t);

extern int mu_body_get_stream     (mu_body_t, mu_stream_t *)
                                      __attribute__ ((deprecated));
extern int mu_body_get_streamref  (mu_body_t body, mu_stream_t *pstream);
extern int mu_body_set_stream     (mu_body_t, mu_stream_t, void *owner);
extern int mu_body_set_get_stream (mu_body_t,
				   int (*) (mu_body_t, mu_stream_t *),
				   void *owner);
  
extern int mu_body_size           (mu_body_t, size_t *);
extern int mu_body_set_size       (mu_body_t,
				int (*_size) (mu_body_t, size_t*), void *owner);
extern int mu_body_lines          (mu_body_t, size_t *);
extern int mu_body_set_lines      (mu_body_t,
				int (*_lines) (mu_body_t, size_t*), void *owner);

#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_BODY_H */
