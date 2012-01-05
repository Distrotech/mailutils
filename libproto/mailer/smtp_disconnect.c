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

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <mailutils/stream.h>
#include <mailutils/sys/smtp.h>

int
mu_smtp_disconnect (mu_smtp_t smtp)
{
  if (smtp == NULL)
    return EINVAL;

  smtp->state = MU_SMTP_INIT;

  if (smtp->rdbuf)
    smtp->rdbuf[0] = 0;

  return mu_stream_close (smtp->carrier);
}
