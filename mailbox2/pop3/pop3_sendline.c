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

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

#include <mailutils/sys/pop3.h>
#include <mailutils/error.h>

/* A socket may write less then expected but stream.c:stream_write() will
   always try to send the entire buffer unless an error is reported.  We have
   to cope with nonblocking, it is done by keeping track with the pop3->ptr
   pointer if the write failed we keep track and restart where we left.  */

int
pop3_send (pop3_t pop3)
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
	  int ready = stream_is_writeready (pop3->carrier, pop3->timeout);
	  if (ready == 0)
	    return MU_ERROR_TIMEOUT;
	}

      status = stream_write (pop3->carrier, pop3->io.buf, len, &n);
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
pop3_writeline (pop3_t pop3, const char *format, ...)
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
pop3_sendline (pop3_t pop3, const char *line)
{
  if (line)
    {
      int status = pop3_writeline (pop3, line);
      if (status)
	return status;
    }
  return pop3_send (pop3);
}
