/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2001, 2005, 2007, 2010-2012 Free Software
   Foundation, Inc.

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
#include <syslog.h>
#include <string.h>
#include <mailutils/error.h>


/* Historic shortcuts for mu_diag_ functions */

int
mu_verror (const char *fmt, va_list ap)
{
  mu_diag_voutput (MU_DIAG_ERROR, fmt, ap);
  return 0;
}

int
mu_error (const char *fmt, ...)
{
  va_list ap;
  va_start (ap, fmt);
  mu_verror (fmt, ap);
  va_end (ap);
  return 0;
}




