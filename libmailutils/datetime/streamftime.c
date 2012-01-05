/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2002, 2007, 2009-2012 Free Software Foundation,
   Inc.

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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mailutils/diag.h>
#include <mailutils/datetime.h>
#include <mailutils/util.h>
#include <mailutils/stream.h>
#include <mailutils/errno.h>
#include <mailutils/cstr.h>
#include <mailutils/cctype.h>

#define ISO_8601_START_WDAY 1 /* Monday */
#define ISO_8601_MAX_WDAY   4 /* Thursday */
#define MAXDAYS 366 /* Max. number of days in a year */
#define ISO_8601_OFF ((MAXDAYS / 7 + 2) * 7)

int
ISO_8601_weekdays (int yday, int wday)
{
  return (yday
	  - (yday - wday + ISO_8601_MAX_WDAY + ISO_8601_OFF) % 7
	  + ISO_8601_MAX_WDAY - ISO_8601_START_WDAY);
}

int
mu_c_streamftime (mu_stream_t str, const char *fmt, struct tm *input_tm,
		  struct mu_timezone *tz)
{
  int rc = 0;
  struct tm tm;

  /* Copy input TM because it might have been received from gmtime and
     the fmt might result in further calls to gmtime which will clobber
     it. */
  tm = *input_tm;
  while (*fmt && rc == 0)
    {
      size_t len = strcspn (fmt, "%");
      if (len)
	{
	  rc = mu_stream_write (str, fmt, len, NULL);
	  if (rc)
	    break;
	}

      fmt += len;

    restart:
      if (!*fmt || !*++fmt)
	break;

      switch (*fmt)
	{
	case 'a':
	  /* The abbreviated weekday name. */
	  if (tm.tm_wday < 0 || tm.tm_wday > 6)
	    rc = ERANGE;
	  else
	    rc = mu_stream_write (str, _mu_datetime_short_wday[tm.tm_wday],
				  strlen (_mu_datetime_short_wday[tm.tm_wday]),
				  NULL);
	  break;
	  
	case 'A':
	  /* The full weekday name. */
	  if (tm.tm_wday < 0 || tm.tm_wday > 6)
	    rc = ERANGE;
	  else
	    rc = mu_stream_write (str, _mu_datetime_full_wday[tm.tm_wday],
				  strlen (_mu_datetime_full_wday[tm.tm_wday]),
				  NULL);
	  break;
	  
	case 'b':
	case 'h':
	  /* The abbreviated month name. */
	  if (tm.tm_mon < 0 || tm.tm_mon > 11)
	    rc = ERANGE;
	  else
	    rc = mu_stream_write (str, _mu_datetime_short_month[tm.tm_mon],
				  strlen (_mu_datetime_short_month[tm.tm_mon]),
				  NULL);
	  break;
	  
	case 'B':
	  /* The full month name. */
	  if (tm.tm_mon < 0 || tm.tm_mon > 11)
	    rc = ERANGE;
	  else
	    rc = mu_stream_write (str, _mu_datetime_full_month[tm.tm_mon],
				  strlen (_mu_datetime_full_month[tm.tm_mon]),
				  NULL);
	  break;
	  
	case 'c':
	  /* The preferred date and time representation. */
	  rc = mu_c_streamftime (str, "%a %b %e %H:%M:%S %Y", &tm, tz);
	  break;
	    
	case 'C':
	  /* The century number (year/100) as a 2-digit integer. */
	  rc = mu_stream_printf (str, "%02d", (tm.tm_year + 1900) / 100);
	  break;
	  
	case 'd':
	  /* The day of the month as a decimal number (range 01 to 31). */
	  if (tm.tm_mday < 1 || tm.tm_mday > 31)
	    rc = ERANGE;
	  else
	    rc = mu_stream_printf (str, "%02d", tm.tm_mday);
	  break;
	  
	case 'D':
	  /* Equivalent to %m/%d/%y. */
	  rc = mu_c_streamftime (str, "%m/%d/%y", &tm, tz);
	  break;
	  
	case 'e':
	  /* Like %d, the day of the month as a decimal number, but a leading
	     zero is replaced by a space. */
	  if (tm.tm_mday < 1 || tm.tm_mday > 31)
	    rc = ERANGE;
	  else
	    rc = mu_stream_printf (str, "%2d", tm.tm_mday);
	  break;
	  
	case 'E':
	  /* Modifier. The Single Unix Specification mentions %Ec, %EC, %Ex,
	     %EX, %Ey, and %EY, which are supposed to use a corresponding
	     locale-dependent alternative representation.
	     A no-op in POSIX locale */
	  goto restart;
	  
	case 'F':
	  /* Equivalent to %Y-%m-%d (the ISO 8601 date format). */
	  rc = mu_c_streamftime (str, "%Y-%m-%d", &tm, tz);
	  break;
	  	  
	case 'V':
	  /* The ISO 8601:1988 week number of the current year as a decimal
	     number range 01 to 53, where week 1 is the first week that has
	     at least 4 days in the current year, and with Monday as the
	     first day of the week.
	  */
	case 'G':
	  /* The ISO 8601 year with century as a decimal number.  The 4-digit
	     year corresponding to the ISO week number (see  %V).  This has
	     the same format and value as %y, except that if the ISO week
	     number belongs to the previous or next year, that year is used
	     instead.
	  */
	case 'g':
	  /* Like  %G, but without century, that is, with a 2-digit year
	     (00-99). */
	  {
	    int year = tm.tm_year + 1900;
	    int days = ISO_8601_weekdays (tm.tm_yday, tm.tm_wday);

	    if (days < 0)
	      {
		days = ISO_8601_weekdays (tm.tm_yday +
					  mu_datetime_year_days (year - 1),
					  tm.tm_wday);
		year--;
	      }
	    else
	      {
		int d = ISO_8601_weekdays (tm.tm_yday -
					   mu_datetime_year_days (year),
					   tm.tm_wday);
		if (d >= 0)
		  {
		    year++;
		    days = d;
		  }
	      }

	    switch (*fmt)
	      {
	      case 'V':
		rc = mu_stream_printf (str, "%02d", days / 7 + 1);
		break;

	      case 'G':
		rc = mu_stream_printf (str, "%4d", year);
		break;

	      case 'g':
		rc = mu_stream_printf (str, "%02d", year % 100);
	      }
	  }
	  break;
	  
	case 'H':
	  /* The hour as a decimal number using a 24-hour clock (range 00 to
	     23). */
	  rc = mu_stream_printf (str, "%02d", tm.tm_hour);
	  break;
	  
	case 'I':
	  /* The hour as a decimal number using a 12-hour clock (range 01 to
	     12). */
	  {
	    unsigned n = tm.tm_hour % 12;
	    rc = mu_stream_printf (str, "%02d", n == 0 ? 12 : n);
	  }
	  break;

	case 'j':
	  /* The day of the year as a decimal number (range 001 to 366). */
	  rc = mu_stream_printf (str, "%03d", tm.tm_yday + 1);
	  break;
	  
	case 'k':
	  /* The hour (24-hour clock) as a decimal number (range 0 to 23);
	     single digits are preceded by a blank. */
	  rc = mu_stream_printf (str, "%2d", tm.tm_hour);
	  break;
	  
	case 'l':
	  /* The hour (12-hour clock) as a decimal number (range 1 to 12);
	     single digits are preceded by a blank. */
	  {
	    unsigned n = tm.tm_hour % 12;
	    rc = mu_stream_printf (str, "%2d", n == 0 ? 12 : n);
	  }
	  break;
	  
	case 'm':
	  /* The month as a decimal number (range 01 to 12). */
	  rc = mu_stream_printf (str, "%02d", tm.tm_mon + 1);
	  break;
				   
	case 'M':
	  /* The minute as a decimal number (range 00 to 59). */
	  rc = mu_stream_printf (str, "%02d", tm.tm_min);
	  break;
	  
	case 'n':
	  /* A newline character. */
	  rc = mu_stream_write (str, "\n", 1, NULL);
	  break;
	  
	case 'O':
	  /* Modifier.  The Single Unix Specification mentions %Od, %Oe, %OH,
	     %OI, %Om, %OM, %OS, %Ou, %OU, %OV, %Ow, %OW, and %Oy, which are
	     supposed to use alternative numeric symbols.

	     Hardly of any use for our purposes, hence a no-op. */
	  goto restart;
	  
	case 'p':
	  /* Either "AM" or "PM" according to the given time value.
	     Noon is treated as "PM" and midnight as "AM". */
	  rc = mu_stream_write (str,
				tm.tm_hour < 12 ? "AM" : "PM",
				2, NULL);
	  break;
				
	case 'P':
	  /* Like %p but in lowercase: "am" or "pm". */
	  rc = mu_stream_write (str,
				tm.tm_hour < 12 ? "am" : "pm",
				2, NULL);
	  break;
	  
	case 'r':
	  /* The time in a.m. or p.m. notation, i.e. %I:%M:%S %p. */
	  rc = mu_c_streamftime (str, "%I:%M:%S %p", &tm, tz);
	  break;
	  
	case 'R':
	  /* The time in 24-hour notation (%H:%M) */
	  rc = mu_c_streamftime (str, "%H:%M", &tm, tz);
	  break;
	  
	case 's':
	  /* The number of seconds since the Epoch */
	  rc = mu_stream_printf (str, "%lu",
				 (unsigned long) mu_datetime_to_utc (&tm, tz));
	  break;
	  
	case 'S':
	  /* The second as a decimal number (range 00 to 60) */
	  rc = mu_stream_printf (str, "%02d", tm.tm_sec);
	  break;
	  
	case 't':
	  /* A tab character. */
	  rc = mu_stream_write (str, "\t", 1, NULL);
	  break;
	  
	case 'T':
	  /* The time in 24-hour notation (%H:%M:%S) */
	  rc = mu_c_streamftime (str, "%H:%M:%S", &tm, tz);
	  break;
	  
	case 'u':
	  /* The day of the week as a decimal, range 1 to 7, Monday being 1.
	   */
	  rc = mu_stream_printf (str, "%1d",
				 tm.tm_wday == 0 ? 7 : tm.tm_wday);
	  break;
	  
	case 'U':
	  /* The week number of the current year as a decimal number, range
	     00 to 53, starting with the first Sunday as the first day of
	     week 01.
	  */
	  rc = mu_stream_printf (str, "%02d",
				 (tm.tm_yday - tm.tm_wday + 7) / 7);
	  break;
	  
	case 'w':
	  /* The day of the week as a decimal, range 0 to 6, Sunday being 0.
	   */
	  rc = mu_stream_printf (str, "%01d", tm.tm_wday);
	  break;
	  
	case 'W':
	  /* The week number of the current year as a decimal number, range
	     00 to 53, starting with the first Monday as the first day of
	     week 01. */
	  rc = mu_stream_printf (str, "%02d", 
			 (tm.tm_yday - (tm.tm_wday - 1 + 7) % 7 + 7) / 7);
	  break;

	  /* The preferred date representation without the time:
	     equivalent to %D */
	case 'x':
	  rc = mu_c_streamftime (str, "%m/%d/%y", &tm, tz);
	  break;
	  
	case 'X':
	  /* The preferred date representation without the date */
	  rc = mu_c_streamftime (str, "%H:%M:%S", &tm, tz);
	  break;

	case 'y':
	  /* The year as a decimal number without a century (range 00 to 99).
	   */
	  rc = mu_stream_printf (str, "%02d", (tm.tm_year + 1900) % 100);
	  break;
	  
	case 'Y':
	  /* The year as a decimal number including the century. */
	  rc = mu_stream_printf (str, "%d", tm.tm_year + 1900);
	  break;
	  
	case 'Z':
	  /* The timezone or name or abbreviation. */
	  if (tz && tz->tz_name)
	    {
	      rc = mu_stream_printf (str, "%s", tz->tz_name);
	      break;
	    }
	  /* fall through */
	case 'z':
	  /* The time-zone as hour offset from GMT, for formatting RFC-822
	     dates (e.g. "%a, %d %b %Y %H:%M:%S %z") */
	  {
	    int utc_off = tz ? tz->utc_offset : mu_utc_offset ();
	    int sign;
	    if (utc_off < 0)
	      {
		sign = '-';
		utc_off = - utc_off;
	      }
	    else
	      sign = '+';
	    utc_off /= 60;
	    rc = mu_stream_printf (str, "%c%02u%02u", sign,
				   utc_off / 60, utc_off % 60);
	  }
	  break;
	  
	case '%':
	  /* A literal '%' character. */
	  rc = mu_stream_write (str, "%", 1, NULL);
	  break;

	case '$':
	  /* Ignored for compatibilty with mu_scan_datetime */
	  break;
	  
	case '+':
	  /* Not supported (date and time in date(1) format. */
	default:
	  rc = mu_stream_write (str, fmt-1, 2, NULL);
	  break;
	}
      fmt++;
    }
  /* Restore input tm */
  *input_tm = tm;
  return rc;
}
