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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include <mailutils/sys/pop3.h>
#include <mailutils/error.h>
#include <mailutils/cctype.h>
#include <mailutils/cstr.h>

int
mu_pop3_carrier_is_ready (mu_stream_t carrier, int flag, int timeout)
{
  struct timeval tv, *tvp = NULL;
  int wflags = flag;
  int status;
  
  if (timeout >= 0)
    {
      tv.tv_sec  = timeout / 100;
      tv.tv_usec = (timeout % 1000) * 1000;
      tvp = &tv;
    }
  status = mu_stream_wait (carrier, &wflags, tvp);
  if (status)
    return 0; /* FIXME: provide a way to return error code! */
  return wflags & flag;
}

/* Read a complete line from the pop server. Transform CRLF to LF, remove
   the stuff byte termination octet ".", put a null in the buffer
   when done.  And Do a select() (stream_is_readready()) for the timeout.  */
/* FIXME: Is it needed? */
int
mu_pop3_getline (mu_pop3_t pop3)
{
  size_t n;
  int status = mu_stream_getline (pop3->carrier, &pop3->rdbuf,
				  &pop3->rdsize, &n);
  if (status == 0)
    {
      if (n == 0)
	return EIO;
      n = mu_rtrim_class (pop3->rdbuf, MU_CTYPE_SPACE);

      /* When examining a multi-line response, the client checks to see if the
	 line begins with the termination octet "."(DOT). If yes and if octets
	 other than CRLF follow, the first octet of the line (the termination
	 octet) is stripped away.  */
      if (n >= 2 &&
	  pop3->rdbuf[0] == '.' &&
	  pop3->rdbuf[1] != '\n')
	memmove (pop3->rdbuf, pop3->rdbuf + 1, n);
    }
  return status;
}

