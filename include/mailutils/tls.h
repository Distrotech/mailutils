/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Library Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA  */

#ifndef _MAILUTILS_TLS_H
#define _MAILUTILS_TLS_H

#ifdef WITH_TLS

#include <mailutils/types.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int tls_stream_create __P((stream_t *stream, 
                                  int in_fd, int out_fd, int flags));

extern int mu_check_tls_environment __P((void));
extern int mu_init_tls_libs __P((void));
extern int mu_deinit_tls_libs __P((void));
  
#ifdef __cplusplus
}
#endif

#endif /* WITH_TLS */
#endif /* _MAILUTILS_TLS_H */

