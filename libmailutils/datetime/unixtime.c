/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2001, 2007, 2009-2012 Free Software Foundation,
   Inc.

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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <time.h>
#include <mailutils/datetime.h>

#define SECS_PER_DAY 86400

#define JD_OF_EPOCH 2440588

/* Convert struct tm into UTC. */
time_t
mu_datetime_to_utc (struct tm *tm, struct mu_timezone *tz)
{
  int jd = mu_datetime_julianday (tm->tm_year + 1900, tm->tm_mon + 1,
				  tm->tm_mday);
  return (jd - JD_OF_EPOCH) * SECS_PER_DAY +
         (tm->tm_hour * 60 + tm->tm_min) * 60 + tm->tm_sec
          - (tz ? tz->utc_offset : 0);
}
