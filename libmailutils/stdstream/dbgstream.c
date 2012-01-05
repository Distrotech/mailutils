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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <mailutils/types.h>
#include <mailutils/stream.h>
#include <mailutils/log.h>
#include <mailutils/stdstream.h>

int
mu_dbgstream_create (mu_stream_t *pstr, int severity)
{
  int rc;
  mu_transport_t trans[2];

  rc = mu_stream_ioctl (mu_strerr, MU_IOCTL_TRANSPORT, MU_IOCTL_OP_GET, trans);
  if (rc)
    return rc;
  rc = mu_log_stream_create (pstr, (mu_stream_t) trans[0]);
  if (rc)
    return rc;
  mu_stream_ioctl (*pstr, MU_IOCTL_LOGSTREAM, MU_IOCTL_LOGSTREAM_SET_SEVERITY, 
                   &severity);
  return 0;
}

  
