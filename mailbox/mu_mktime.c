/* Copyright (C) 2001 Free Software Foundation, Inc.
   A wrapper for mktime function allowing to specify the timezone.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the GNU C Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <time.h>

/* Convert struct tm into time_t, taking into account timezone offset */
time_t
mu_mktime (struct tm *timeptr, int tz)
{
  static int mu_timezone;
  
  if (mu_timezone == 0)
    {
	struct tm *tm;
	time_t t = 0;
	tm = gmtime(&t);
	mu_timezone = mktime(tm);
    }
  return mktime(timeptr) - mu_timezone + tz;
}
