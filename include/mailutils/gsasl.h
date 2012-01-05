/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003-2005, 2007-2012 Free Software Foundation, Inc.

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

#ifndef _MAILUTILS_GSASL_H
#define _MAILUTILS_GSASL_H

struct mu_gsasl_module_data
{
  int enable;
  char *service;
  char *realm;
  char *hostname;
  char *anon_user;
  char *cram_md5_pwd;
};

int mu_gsasl_enabled (void);

#ifdef WITH_GSASL
#include <gsasl.h>

int mu_gsasl_module_init (enum mu_gocs_op, void *);
extern struct mu_gsasl_module_data mu_gsasl_module_data;


int gsasl_encoder_stream (mu_stream_t *pstr, mu_stream_t transport,
			  Gsasl_session *ctx, int flags);
int gsasl_decoder_stream (mu_stream_t *pstr, mu_stream_t transport,
			  Gsasl_session *ctx, int flags);

#endif

#endif /* not _MAILUTILS_GSASL_H */
