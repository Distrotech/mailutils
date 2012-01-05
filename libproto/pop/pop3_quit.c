/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2001, 2007, 2010-2012 Free Software Foundation,
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
#include <errno.h>
#include <mailutils/sys/pop3.h>

int
mu_pop3_quit (mu_pop3_t pop3)
{
  int status;

  if (pop3 == NULL)
    return EINVAL;

  switch (pop3->state)
    {
    case MU_POP3_NO_STATE:
      status = mu_pop3_writeline (pop3, "QUIT\r\n");
      MU_POP3_CHECK_ERROR (pop3, status);
      MU_POP3_FCLR (pop3, MU_POP3_ACK);
      pop3->state = MU_POP3_QUIT;

    case MU_POP3_QUIT:
      status = mu_pop3_response (pop3, NULL);
      MU_POP3_CHECK_EAGAIN (pop3, status);
      MU_POP3_CHECK_OK (pop3);
      pop3->state = MU_POP3_NO_STATE;
      _mu_pop3_init (pop3);
      break;

    default:
      status = EINPROGRESS;
    }

  return status;
}
