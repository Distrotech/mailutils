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
#include <stdlib.h>
#include <mailutils/datetime.h>

static int  month_start[]=
    {    0, 31, 59,  90, 120, 151, 181, 212, 243, 273, 304, 334, 365 };
     /* Jan Feb Mar  Apr  May  Jun  Jul  Aug  Sep  Oct  Nov  Dec
         31  28  31   30   31   30   31   31   30   31   30   31
     */

#define leap_year(y) ((y) % 4 == 0 && ((y) % 100 != 0 || (y) % 400 == 0))

/* Compute number of days since January 1 to the given day of year. */
int
mu_datetime_dayofyear (int year, int month, int day)
{
  int  leap, month_days;

  if (year < 0 || month < 1 || month > 12 || day < 1)
    return -1;
    
  leap = leap_year (year);
  
  month_days = month_start[month] - month_start[month - 1]
               + ((month == 2) ? leap : 0);

  if (day > month_days)
    return -1;

  if (month <= 2)
    leap = 0;

  return month_start[month-1] + day + leap;
}
