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

#include <stdlib.h>
#include <mailutils/sys/pop3.h>

int
pop3_uidl_all (pop3_t pop3, iterator_t *piterator)
{
  int status;

  if (pop3 == NULL || piterator == NULL)
    return MU_ERROR_INVALID_PARAMETER;

  switch (pop3->state)
    {
    case POP3_NO_STATE:
      status = pop3_writeline (pop3, "UIDL\r\n");
      POP3_CHECK_ERROR (pop3, status);
      pop3_debug_cmd (pop3);
      pop3->state = POP3_UIDL;

    case POP3_UIDL:
      status = pop3_send (pop3);
      POP3_CHECK_EAGAIN (pop3, status);
      pop3->acknowledge = 0;
      pop3->state = POP3_UIDL_ACK;

    case POP3_UIDL_ACK:
      status = pop3_response (pop3, NULL, 0, NULL);
      POP3_CHECK_EAGAIN (pop3, status);
      pop3_debug_ack (pop3);
      POP3_CHECK_OK (pop3);
      pop3->state = POP3_UIDL_RX;

    case POP3_UIDL_RX:
      status = pop3_iterator_create (pop3, piterator);
      POP3_CHECK_ERROR (pop3, status);
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

int
pop3_uidl_current (iterator_t iterator, unsigned int *pno, char **puidl)
{
  char *buf;
  int status = iterator_current (iterator, (void *)&buf);
  if (status == 0)
    {
      char *space;
      unsigned int msgno = 0;
      /* The format is: msgno uidlstring  */
      space = strchr (buf, ' ');
      if (space)
	{
	  *space++ = '\0';
	  msgno = strtoul (buf, NULL, 10);
	}
      if (space && space[strlen (space) - 1] == '\n')
	space[strlen (space) - 1] = '\0';
      if (space == NULL)
	space = (char *)"";
      if (pno)
	*pno = msgno;
      if (puidl)
	*puidl = strdup (space);
      free (buf);
    }
  return status;
}
