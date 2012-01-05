/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2010-2012 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>. */

#ifndef _MAILUTILS_SYS_TLS_STREAM_H
# define _MAILUTILS_SYS_TLS_STREAM_H

# include <mailutils/types.h>
# include <mailutils/stream.h>
# include <mailutils/sys/stream.h>

enum _mu_tls_stream_state
  {
    state_init,
    state_open,
    state_closed,
    state_destroyed
  };

struct _mu_tls_io_stream
{
  struct _mu_stream stream;
  mu_stream_t transport;
  struct _mu_tls_stream *up;
};

struct _mu_tls_stream
{
  struct _mu_stream stream;
  enum _mu_tls_stream_state state;
  gnutls_session session;
  int tls_err;
  mu_stream_t transport[2];
};

#endif
