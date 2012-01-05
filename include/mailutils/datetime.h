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

#ifndef _MAILUTILS_DATETIME_H
#define _MAILUTILS_DATETIME_H

#ifdef __cplusplus
extern "C" {
#endif

#include <time.h>
#include <mailutils/types.h>

  /* ----------------------- */
  /* Date & time functions   */
  /* ----------------------- */

/* Argument ranges:

   year != 0, AD if > 0, BC if < 0
   1 <= month <= 12
   1 <= day <= maxday(month)
*/
/* Compute Julian Day for the given date */
int mu_datetime_julianday (int year, int month, int day);
/* Compute day of week (Sunday - 0) */
int mu_datetime_dayofweek (int year, int month, int day);
/* Compute ordinal date (1-based) */
int mu_datetime_dayofyear (int year, int month, int day);
/* Return number of days in the year */
int mu_datetime_year_days (int year);

/* Day of week and month names in C locale */
extern const char *_mu_datetime_short_month[];
extern const char *_mu_datetime_full_month[];
extern const char *_mu_datetime_short_wday[];
extern const char *_mu_datetime_full_wday[];


struct mu_timezone
{
  int utc_offset;  /* Seconds east of UTC. */

  const char *tz_name;
    /* Nickname for this timezone, if known. It is always considered
       to be a pointer to static string, so will never be freed. */
};

int mu_parse_date (const char *p, time_t *rettime, const time_t *now);

int mu_utc_offset (void);
void mu_datetime_tz_local (struct mu_timezone *tz);
time_t mu_datetime_to_utc (struct tm *timeptr, struct mu_timezone *tz);
size_t mu_strftime (char *s, size_t max, const char *format, struct tm *tm);

int mu_c_streamftime (mu_stream_t str, const char *fmt, struct tm *tm,
		      struct mu_timezone *tz);
int mu_scan_datetime (const char *input, const char *fmt, struct tm *tm,
		      struct mu_timezone *tz, char **endp);

  /* Common datetime formats: */
#define MU_DATETIME_FROM         "%a %b %e %H:%M:%S %Y"
/* Length of an envelope date in C locale,
   not counting the terminating nul character */
#define MU_DATETIME_FROM_LENGTH 24

#define MU_DATETIME_IMAP         "%d-%b-%Y %H:%M:%S %z"
#define MU_DATETIME_INTERNALDATE "%d-%b-%Y%$ %H:%M:%S %z"

  /* RFC2822 date.  Scan format contains considerable allowances which would
     stun formatting functions, therefore two distinct formats are provided:
     one for outputting and one for scanning: */
#define MU_DATETIME_FORM_RFC822  "%a, %e %b %Y %H:%M:%S %z"
#define MU_DATETIME_SCAN_RFC822  "%[%a, %]%e %b %Y %H:%M%[:%S%] %z"

#ifdef __cplusplus
}
#endif
  
#endif
