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

#include <string.h>
#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif

#include <stdlib.h>
#include <mailutils/sys/pop3.h>

int
pop3_uidl (pop3_t pop3, unsigned msgno, char **uidl)
{
  int status = 0;

  if (pop3 == NULL)
    return MU_ERROR_INVALID_PARAMETER;

  switch (pop3->state)
    {
    case POP3_NO_STATE:
      if (uidl == NULL)
	return MU_ERROR_INVALID_PARAMETER;
      status = pop3_writeline (pop3, "UIDL %d\r\n", msgno);
      POP3_CHECK_ERROR (pop3, status);
      pop3->state = POP3_UIDL;

    case POP3_UIDL:
      status = pop3_send (pop3);
      POP3_CHECK_EAGAIN (pop3, status);
      pop3->acknowledge = 0;
      pop3->state = POP3_UIDL_ACK;

    case POP3_UIDL_ACK:
      status = pop3_response (pop3, NULL, 0, NULL);
      POP3_CHECK_EAGAIN (pop3, status);
      POP3_CHECK_OK (pop3);
      pop3->state = POP3_NO_STATE;

      *uidl = NULL;
      {
	char *space;
	/* Pass the "+OK".  */
	space = strchr (pop3->ack.buf, ' ');
	if (space)
	  {
	    /* Pass the number.  */
	    space = strchr (space, ' ');
	    if (space)
	      {
		size_t len = strlen (space);
		if (space[len - 1] == '\n')
		  {
		    space[len - 1] = '\0';
		    len--;
		  }
		*uidl = calloc (len + 1, 1);
		if (*uidl)
		  memcpy (*uidl, space, len);
	      }
	  }
      }
      if (*uidl == NULL) /* What can we do?  */
	{
	  *uidl = malloc (1);
	  if (*uidl)
	    **uidl = '\0';
	  else
	    status = MU_ERROR_NO_MEMORY;
	}
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
