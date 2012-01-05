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
#include <mailutils/list.h>
#include <mailutils/smtp.h>
#include <mailutils/stream.h>
#include <mailutils/sys/smtp.h>

int
mu_smtp_set_carrier (mu_smtp_t smtp, mu_stream_t carrier)
{
  /* Sanity checks.  */
  if (smtp == NULL)
    return EINVAL;

  if (smtp->carrier)
    {
      /* Close any old carrier.  */
      mu_smtp_disconnect (smtp);
      mu_stream_destroy (&smtp->carrier);
    }
  mu_stream_ref (carrier);
  smtp->carrier = carrier;
  if (MU_SMTP_FISSET (smtp, _MU_SMTP_TRACE))
    _mu_smtp_trace_enable (smtp);
  return 0;
}

int
mu_smtp_get_carrier (mu_smtp_t smtp, mu_stream_t *pcarrier)
{
  /*FIXME*/
  return ENOSYS;
}
