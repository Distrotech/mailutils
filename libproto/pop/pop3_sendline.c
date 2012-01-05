/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003, 2005, 2007, 2010-2012 Free Software Foundation,
   Inc.

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
#include <stdio.h>
#include <stdarg.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

#include <errno.h>
#include <mailutils/sys/pop3.h>

int
mu_pop3_writeline (mu_pop3_t pop3, const char *format, ...)
{
  int status;
  va_list ap;

  va_start (ap, format);
  status = mu_stream_vprintf (pop3->carrier, format, ap);
  va_end(ap);
  return status;
}

int
mu_pop3_sendline (mu_pop3_t pop3, const char *line)
{
  if (line)
    return mu_stream_write (pop3->carrier, line, strlen (line), NULL);
  return mu_stream_flush (pop3->carrier);
}

