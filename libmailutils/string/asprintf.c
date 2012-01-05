/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2009-2012 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library.  If not, see <http://www.gnu.org/licenses/>. */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

#include <mailutils/io.h>

int
mu_asprintf (char **pbuf, const char *fmt, ...)
{
  va_list ap;
  size_t size;
  int rc;
  
  va_start (ap, fmt);
  *pbuf = NULL;
  size = 0;
  rc = mu_vasnprintf (pbuf, &size, fmt, ap);
  va_end (ap);
  return rc;
}
