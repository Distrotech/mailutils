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

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <mailutils/errno.h>
#include <mailutils/filter.h>
#include <mailutils/list.h>
#include <mailutils/smtp.h>
#include <mailutils/stream.h>
#include <mailutils/sys/smtp.h>

static int
_smtp_data_send (mu_smtp_t smtp, mu_stream_t stream)
{
  int status = _mu_smtp_data_begin (smtp);

  if (status)
    return status;

  status = mu_stream_copy (smtp->carrier, stream, 0, NULL);
  _mu_smtp_data_end (smtp);
  return status;
}


int
mu_smtp_send_stream (mu_smtp_t smtp, mu_stream_t stream)
{
  int status;
  mu_stream_t input;
  
  if (!smtp)
    return EINVAL;
  if (MU_SMTP_FISSET (smtp, _MU_SMTP_ERR))
    return MU_ERR_FAILURE;
  if (smtp->state != MU_SMTP_MORE)
    return MU_ERR_SEQ;

  status = mu_filter_create (&input, stream, "CRLFDOT", MU_FILTER_ENCODE,
			     MU_STREAM_READ);
  if (status)
    return status;
  
  status = _smtp_data_send (smtp, input);
  mu_stream_destroy (&input);
  return status;
}
