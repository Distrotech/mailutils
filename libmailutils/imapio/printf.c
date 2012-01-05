/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2011-2012 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>. */

#include <config.h>
#include <stdarg.h>
#include <mailutils/types.h>
#include <mailutils/imapio.h>
#include <mailutils/stream.h>
#include <mailutils/sys/imapio.h>

int
mu_imapio_printf (mu_imapio_t io, const char *fmt, ...)
{
  va_list ap;
  int status;
  
  va_start (ap, fmt);
  status = mu_stream_vprintf (io->_imap_stream, fmt, ap);
  va_end (ap);
  return status;
}
    
