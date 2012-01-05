/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2001, 2007, 2009-2012 Free Software Foundation,
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

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <mailutils/sys/pop3.h>

int
mu_pop3_stat (mu_pop3_t pop3, size_t *msg_count, mu_off_t *size)
{
  int status;
  unsigned long lv, count;
  
  if (pop3 == NULL || msg_count == NULL)
    return EINVAL;
  if (size == NULL)
    return MU_ERR_OUT_PTR_NULL;

  switch (pop3->state)
    {
    case MU_POP3_NO_STATE:
      status = mu_pop3_writeline (pop3, "STAT\r\n");
      MU_POP3_CHECK_ERROR (pop3, status);
      MU_POP3_FCLR (pop3, MU_POP3_ACK);
      pop3->state = MU_POP3_STAT;

    case MU_POP3_STAT:
      status = mu_pop3_response (pop3, NULL);
      MU_POP3_CHECK_EAGAIN (pop3, status);
      MU_POP3_CHECK_OK (pop3);
      pop3->state = MU_POP3_NO_STATE;

      /* Parse the answer.  */
      *msg_count = 0;
      lv = 0;
      /* FIXME: Error checking */
      sscanf (pop3->ackbuf, "+OK %lu %lu", &count, &lv);
      *msg_count = count;
      *size = lv;
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
