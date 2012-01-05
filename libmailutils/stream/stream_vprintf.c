/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2009-2012 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>. */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <mailutils/types.h>
#include <mailutils/stream.h>
#include <mailutils/io.h>
#include <stdlib.h>
#include <string.h>

int
mu_stream_vprintf (mu_stream_t str, const char *fmt, va_list ap)
{
  char *buf = NULL;
  size_t buflen = 0;
  size_t n;
  int rc;

  rc = mu_vasnprintf (&buf, &buflen, fmt, ap);
  if (rc)
    return rc;
  n = strlen (buf);
  rc = mu_stream_write (str, buf, n, NULL);
  free (buf);
  return rc;
}

