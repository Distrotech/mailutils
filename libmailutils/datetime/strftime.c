/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2011-2012 Free Software Foundation, Inc.

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
#include <mailutils/datetime.h>
#include <mailutils/stream.h>
#include <mailutils/errno.h>
#include <mailutils/cstr.h>

/* A locale-independent version of strftime */
size_t
mu_strftime (char *buf, size_t size, const char *format, struct tm *tm)
{
  int rc;
  mu_stream_t str;
  mu_stream_stat_buffer stat;

  if (mu_fixed_memory_stream_create (&str, buf, size, MU_STREAM_WRITE))
    return 0;
  mu_stream_set_stat (str, MU_STREAM_STAT_MASK (MU_STREAM_STAT_OUT), stat);
  rc = mu_c_streamftime (str, format, tm, NULL);
  if (rc == 0)
    rc = mu_stream_write (str, "", 1, NULL);
  mu_stream_unref (str);
  return rc ? 0 : stat[MU_STREAM_STAT_OUT] - 1;
}
  

