/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2001, 2003, 2007, 2010-2012 Free Software
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <mailutils/sys/pop3.h>

int
mu_pop3_disconnect (mu_pop3_t pop3)
{
  /* Sanity checks.  */
  if (pop3 == NULL)
    return EINVAL;

  /* We can keep some of the fields, if they decide to pop3_connect() again but
     clear the states.  */
  pop3->state = MU_POP3_NO_STATE;
  MU_POP3_FCLR (pop3, MU_POP3_ACK);

  if (pop3->rdbuf)
    pop3->rdbuf[0] = 0;

  /* Free the timestamp, it will be different on each connection.  */
  if (pop3->timestamp)
    {
      free (pop3->timestamp);
      pop3->timestamp = NULL;
    }

  /* Close the stream.  */
  if (mu_stream_is_open (pop3->carrier))
    return mu_stream_close (pop3->carrier);
  return 0;
}
