/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2001, 2007, 2009-2012, 2014-2015 Free Software
   Foundation, Inc.

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

#define TMSEC(t) (((t)->tm_hour * 60 + (t)->tm_min) * 60 + (t)->tm_sec)

/* Returns the offset of our timezone from UTC, in seconds. */
int
mu_utc_offset (void)
{
  time_t t = time (NULL);
  struct tm ltm = *localtime (&t);
  struct tm gtm = *gmtime (&t);
  int d = TMSEC (&ltm) - TMSEC (&gtm);
  if (!(ltm.tm_year = gtm.tm_year && ltm.tm_mon == gtm.tm_mon))
    d += 86400;
  return d;
}
