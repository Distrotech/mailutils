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
#include <mailutils/sys/pop3.h>

/* If we did not grap the ack already, call pop3_readline() but handle
   Nonblocking also.  */
int
pop3_response (pop3_t pop3, char *buffer, size_t buflen, size_t *pnread)
{
  size_t n = 0;
  int status = 0;

  if (!pop3->acknowledge)
    {
      size_t len = pop3->ack.len - (pop3->ack.ptr  - pop3->ack.buf);
      status = pop3_readline (pop3, pop3->ack.ptr, len, &n);
      pop3->ack.ptr += n;
      if (status != 0)
	return status;
      pop3->acknowledge = 1; /* Flag that we have the ack.  */
      pop3->ack.ptr = pop3->ack.buf;
    }
  else
    n = strlen (pop3->ack.buf);

  if (buffer)
    {
      buflen--; /* Leave space for the NULL.  */
      n = (buflen < n) ? buflen : n;
      memcpy (buffer, pop3->ack.buf, n);
      buffer[n] = '\0';
    }

  if (pnread)
    *pnread = n;
  return status;
}
