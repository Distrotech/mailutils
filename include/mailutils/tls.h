/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003 Free Software Foundation, Inc.

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

#ifndef _MAILUTILS_TLS_H
#define _MAILUTILS_TLS_H

#ifdef WITH_TLS

#include <mailutils/types.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int tls_stream_create __P((stream_t *stream, 
                                  stream_t strin, stream_t strout, int flags));
extern int tls_stream_create_client __P((stream_t *stream,
					 stream_t strin, stream_t strout,
					 int flags));
extern int tls_stream_create_client_from_tcp __P((stream_t *stream,
						  stream_t tcp_str,
						  int flags));

extern int mu_check_tls_environment __P((void));
extern int mu_init_tls_libs __P((void));
extern void mu_deinit_tls_libs __P((void));
extern void mu_tls_init_argp __P((void));
extern void mu_tls_init_client_argp __P((void));

extern int mu_tls_enable;
  
#ifdef __cplusplus
}
#endif

#endif /* WITH_TLS */
#endif /* _MAILUTILS_TLS_H */

