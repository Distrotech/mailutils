/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2004 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <string.h>
#include <errno.h>
#include <mailutils/sys/nntp.h>

/* If we did not grap the ack already, call nntp_readline() but handle
   Nonblocking also.  */
int
mu_nntp_response (mu_nntp_t nntp, char *buffer, size_t buflen, size_t *pnread)
{
  size_t n = 0;
  int status = 0;

  if (nntp == NULL)
    return EINVAL;

  if (!nntp->acknowledge)
    {
      size_t len = nntp->ack.len - (nntp->ack.ptr  - nntp->ack.buf);
      status = mu_nntp_readline (nntp, nntp->ack.ptr, len, &n);
      nntp->ack.ptr += n;
      if (status == 0)
	{
	  len = nntp->ack.ptr - nntp->ack.buf;
	  if (len && nntp->ack.buf[len - 1] == '\n')
	    nntp->ack.buf[len - 1] = '\0';
	  nntp->acknowledge = 1; /* Flag that we have the ack.  */
	  nntp->ack.ptr = nntp->ack.buf;
	}
      else
	{
	  /* Provide them with an error.  */
	  const char *econ = "500 NNTP IO ERROR";
	  n = strlen (econ);
	  strcpy (nntp->ack.buf, econ);
	}
    }
  else
    n = strlen (nntp->ack.buf);

  if (buffer)
    {
      buflen--; /* Leave space for the NULL.  */
      n = (buflen < n) ? buflen : n;
      memcpy (buffer, nntp->ack.buf, n);
      buffer[n] = '\0';
    }

  if (pnread)
    *pnread = n;
  return status;
}
