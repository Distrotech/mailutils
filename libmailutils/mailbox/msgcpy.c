/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2002, 2004-2007, 2009-2012 Free Software
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

#include <config.h>
#include <stdlib.h>

#include <mailutils/types.h>
#include <mailutils/message.h>
#include <mailutils/errno.h>
#include <mailutils/stream.h>
#include <mailutils/sys/message.h>

int
mu_message_create_copy (mu_message_t *to, mu_message_t from)
{
  int status = 0;
  mu_stream_t fromstr = NULL;
  mu_stream_t tmp = NULL;

  if (!to)
    return MU_ERR_OUT_PTR_NULL;
  if (!from)
    return EINVAL;

  status = mu_memory_stream_create (&tmp, MU_STREAM_RDWR|MU_STREAM_SEEK);
  if (status)
    return status;

  status = mu_message_get_streamref (from, &fromstr);
  if (status)
    {
      mu_stream_destroy (&tmp);
      return status;
    }

  status = mu_stream_copy (tmp, fromstr, 0, NULL);
  if (status == 0)
    {
      status = mu_message_create (to, NULL);
      if (status == 0)
	mu_message_set_stream (*to, tmp, NULL);
    }

  if (status)
    mu_stream_destroy (&tmp);
  mu_stream_destroy (&fromstr);

  return status;
}






















