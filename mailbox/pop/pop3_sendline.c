/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003 Free Software Foundation, Inc.

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

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

#include <errno.h>
#include <mailutils/sys/pop3.h>

static int mu_pop3_carrier_is_write_ready (stream_t carrier, int timeout);

/* A socket may write less then expected but stream.c:stream_write() will
   always try to send the entire buffer unless an error is reported.  We have
   to cope with nonblocking, it is done by keeping track with the pop3->ptr
   pointer if the write failed we keep track and restart where we left.  */
int
mu_pop3_send (mu_pop3_t pop3)
{
  int status = 0;
  if (pop3->carrier && (pop3->io.ptr > pop3->io.buf))
    {
      size_t n = 0;
      size_t len = pop3->io.ptr - pop3->io.buf;

      /* Timeout with select(), note that we have to reset select()
	 since on linux tv is modified when error.  */
      if (pop3->timeout)
	{
	  int ready = mu_pop3_carrier_is_write_ready (pop3->carrier, pop3->timeout);
	  if (ready == 0)
	    return ETIMEDOUT;
	}

      status = stream_write (pop3->carrier, pop3->io.buf, len, 0, &n);
      if (n)
	{
	  /* Consume what we sent.  */
	  memmove (pop3->io.buf, pop3->io.buf + n, len - n);
	  pop3->io.ptr -= n;
	}
    }
  else
    pop3->io.ptr = pop3->io.buf;
  return status;
}

/* According to RFC 2449: The maximum length of a command is increased from
   47 characters (4 character command, single space, 40 character argument,
   CRLF) to 255 octets, including the terminating CRLF.  But we are flexible
   on this and realloc() as needed. NOTE: The terminated CRLF is not
   included.  */
int
mu_pop3_writeline (mu_pop3_t pop3, const char *format, ...)
{
  int len;
  va_list ap;
  int done = 1;

  va_start(ap, format);
  /* C99 says that a conforming implementation of snprintf () should
     return the number of char that would have been call but many old
     GNU/Linux && BSD implementations return -1 on error.  Worse,
     QnX/Neutrino actually does not put the terminal null char.  So
     let's try to cope.  */
  do
    {
      len = vsnprintf (pop3->io.buf, pop3->io.len - 1, format, ap);
      if (len < 0 || len >= (int)pop3->io.len
	  || !memchr (pop3->io.buf, '\0', len + 1))
	{
	  pop3->io.len *= 2;
	  pop3->io.buf = realloc (pop3->io.buf, pop3->io.len);
	  if (pop3->io.buf == NULL)
	    return ENOMEM;
	  done = 0;
	}
      else
	done = 1;
    }
  while (!done);
  va_end(ap);
  pop3->io.ptr = pop3->io.buf + len;
  return 0;
}

int
mu_pop3_sendline (mu_pop3_t pop3, const char *line)
{
  if (line)
    {
      int status = mu_pop3_writeline (pop3, line);
      if (status)
	return status;
    }
  return mu_pop3_send (pop3);
}

static int
mu_pop3_carrier_is_write_ready (stream_t carrier, int timeout)
{
  int fd = -1;
  int ready = 0;

  stream_get_fd (carrier, &fd);
 
  if (fd >= 0)
    {
      struct timeval tv;
      fd_set fset;
                                                                                                                             
      FD_ZERO (&fset);
      FD_SET (fd, &fset);
                                                                                                                             
      tv.tv_sec  = timeout / 100;
      tv.tv_usec = (timeout % 1000) * 1000;
                                                                                                                             
      ready = select (fd + 1, NULL, &fset, NULL, (timeout == -1) ? NULL: &tv);
      ready = (ready == -1) ? 0 : 1;
    }
  return ready;
}

