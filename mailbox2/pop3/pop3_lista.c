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
#include <stdlib.h>
#include <mailutils/sys/pop3.h>

int
pop3_list_all (pop3_t pop3, iterator_t *piterator)
{
  int status;

  if (pop3 == NULL || piterator == NULL)
    return MU_ERROR_INVALID_PARAMETER;

  switch (pop3->state)
    {
    case POP3_NO_STATE:
      status = pop3_writeline (pop3, "LIST\r\n");
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
      POP3_CHECK_EAGAIN (pop3, status);
      pop3_debug_cmd (pop3);
      POP3_CHECK_OK (pop3);
      pop3->state = POP3_LIST_RX;

    case POP3_LIST_RX:
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
pop3_list_first (pop3_list_iterator_t iterator)
{
  return iterator_first (iterator);
}

int
pop3_list_next (pop3_list_iterator_t iterator)
{
  return iterator_next (iterator);
}

int
pop3_list_is_done (pop3_list_iterator_t iterator)
{
  return iterator_is_done (iterator);
}

void
pop3_list_destroy (pop3_list_iterator_t *piterator)
{
  iterator_destroy (piterator);
}

int
pop3_list_current (pop3_list_iterator_t iterator, unsigned int *pno,
		   size_t *plen)
{
  char *buf;
  int status = iterator_current (iterator, (void *)&buf);
  if (status == 0)
    {
      size_t size;
      unsigned int msgno;
      size = msgno = 0;
      sscanf (buf, "%d %d", &msgno, &size);
      if (pno)
	*pno = msgno;
      if (plen)
	*plen = size;
      free (buf);
    }
  return status;
}
