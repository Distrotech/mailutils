/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003, 2007, 2010-2012 Free Software Foundation, Inc.

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
mu_pop3_pass (mu_pop3_t pop3, const char *passwd)
{
  int status;
  
  if (pop3 == NULL || passwd == NULL)
    return EINVAL;

  switch (pop3->state)
    {
    case MU_POP3_NO_STATE:
      if (mu_pop3_trace_mask (pop3, MU_POP3_TRACE_QRY, MU_XSCRIPT_SECURE))
	_mu_pop3_xscript_level (pop3, MU_XSCRIPT_SECURE);
      status = mu_pop3_writeline (pop3, "PASS %s\r\n", passwd);
      _mu_pop3_xscript_level (pop3, MU_XSCRIPT_NORMAL);
      MU_POP3_CHECK_ERROR (pop3, status);
      /* FIXME: how to obscure the passwd in the stream buffer? */
      MU_POP3_FCLR (pop3, MU_POP3_ACK);
      pop3->state = MU_POP3_PASS;

    case MU_POP3_PASS:
      status = mu_pop3_response (pop3, NULL);
      MU_POP3_CHECK_EAGAIN (pop3, status);
      MU_POP3_CHECK_OK (pop3);
      pop3->state = MU_POP3_NO_STATE;
      break;

      /* They must deal with the error first by reopening.  */
    case MU_POP3_ERROR:
      status = ECANCELED;
      break;

    default:
      status = EINPROGRESS;
    }

  return status;
}
