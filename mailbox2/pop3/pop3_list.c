/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Library Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#ifdef HAVE_STRING_H
# include <string.h>
#else
# include <strings.h>
#endif

#include <stdio.h>
#include <mailutils/sys/pop3.h>

int
pop3_list (pop3_t pop3, unsigned int msgno, size_t *psize)
{
  int status;

  if (pop3 == NULL || msgno == 0 || psize == NULL)
    return MU_ERROR_INVALID_PARAMETER;

  switch (pop3->state)
    {
    case POP3_NO_STATE:
      if (psize == NULL)
	return MU_ERROR_INVALID_PARAMETER;
      status = pop3_writeline (pop3, "LIST %d\r\n", msgno);
      POP3_CHECK_ERROR (pop3, status);
      pop3_debug_cmd (pop3);
      pop3->state = POP3_LIST;

    case POP3_LIST:
      status = pop3_send (pop3);
      POP3_CHECK_EAGAIN (pop3, status);
      pop3->acknowledge = 0;
      pop3->state = POP3_LIST_ACK;

    case POP3_LIST_ACK:
      status = pop3_response (pop3, NULL, 0, NULL);
      pop3_debug_ack (pop3);
      POP3_CHECK_EAGAIN (pop3, status);
      POP3_CHECK_OK (pop3);
      pop3->state = POP3_NO_STATE;

      /* Parse the answer.  */
      *psize = 0;
      sscanf (pop3->ack.buf, "+OK %d %d", &msgno, psize);
      break;

      /* They must deal with the error first by reopening.  */
    case POP3_ERROR:
      status = MU_ERROR_OPERATION_CANCELED;
      break;

    default:
      status = MU_ERROR_OPERATION_IN_PROGRESS;
    }

  return status;
}
