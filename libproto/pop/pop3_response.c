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

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <mailutils/cctype.h>
#include <mailutils/cstr.h>
#include <mailutils/sys/pop3.h>

#define POP3_DEFERR "-ERR POP3 IO ERROR"

/* If we did not grap the ack already, call pop3_readline() but handle
   Nonblocking also.  */
int
mu_pop3_response (mu_pop3_t pop3, size_t *pnread)
{
  size_t n = 0;
  int status = 0;

  if (pop3 == NULL)
    return EINVAL;

  if (!MU_POP3_FISSET (pop3, MU_POP3_ACK))
    {
      status = mu_stream_getline (pop3->carrier, &pop3->ackbuf,
				  &pop3->acksize, NULL);
      if (status == 0)
	{
	  n = mu_rtrim_class (pop3->ackbuf, MU_CTYPE_SPACE);
	  MU_POP3_FSET (pop3, MU_POP3_ACK); /* Flag that we have the ack.  */
	}
      else
	{
	  /* Provide them with an error.  */
	  if (pop3->acksize < sizeof (POP3_DEFERR))
	    {
	      char *p = realloc (pop3->ackbuf, sizeof (POP3_DEFERR));
	      if (p)
		{
		  pop3->ackbuf = p;
		  pop3->acksize = sizeof (POP3_DEFERR);
		}
	    }
	  if (pop3->ackbuf)
	    strncpy (pop3->ackbuf, POP3_DEFERR, pop3->acksize);
	}
    }
  else if (pop3->ackbuf)
    n = strlen (pop3->ackbuf);

  if (n < 3)
    status = MU_ERR_BADREPLY;
  else if (strncmp (pop3->ackbuf, "-ERR", 4) == 0)
    status = MU_ERR_REPLY;
  else if (strncmp (pop3->ackbuf, "+OK", 3))
    status = MU_ERR_BADREPLY;
  
  if (pnread)
    *pnread = n;
  return status;
}
