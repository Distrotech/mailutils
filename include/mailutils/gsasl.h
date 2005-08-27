/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003, 2004, 2005 Free Software Foundation, Inc.

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

#ifndef _MAILUTILS_GSASL_H
#define _MAILUTILS_GSASL_H

#include <gsasl.h>

int mu_gsasl_stream_create (mu_stream_t *stream, mu_stream_t transport,
			 Gsasl_session_ctx *ctx,
			 int flags);

void mu_gsasl_init_argp (void);

extern char *mu_gsasl_cram_md5_pwd;

#endif /* not _MAILUTILS_GSASL_H */
