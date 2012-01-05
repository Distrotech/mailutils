/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003-2004, 2007, 2010-2012 Free Software Foundation,
   Inc.

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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <mailutils/sys/pop3.h>

int
mu_pop3_set_carrier (mu_pop3_t pop3, mu_stream_t carrier)
{
  /* Sanity checks.  */
  if (pop3 == NULL)
    return EINVAL;

  if (pop3->carrier)
    {
      /* Close any old carrier.  */
      mu_pop3_disconnect (pop3);
      mu_stream_destroy (&pop3->carrier);
    }
  mu_stream_ref (carrier);
  pop3->carrier = carrier;
  if (MU_POP3_FISSET (pop3, MU_POP3_TRACE))
    _mu_pop3_trace_enable (pop3);
  pop3->state = MU_POP3_CONNECT;
  return 0;
}

/* FIXME: Is it needed? */
int
mu_pop3_get_carrier (mu_pop3_t pop3, mu_stream_t *pcarrier)
{
  /* Sanity checks.  */
  if (pop3 == NULL)
    return EINVAL;
  if (pcarrier == NULL)
    return MU_ERR_OUT_PTR_NULL;

  mu_stream_ref (pop3->carrier);
  *pcarrier = pop3->carrier;
  return 0;
}
