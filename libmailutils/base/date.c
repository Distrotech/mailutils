/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002, 2007, 2009, 2010, 2011 Free
   Software Foundation, Inc.

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
#include <mailutils/util.h>
#include <mailutils/stream.h>
#include <mailutils/errno.h>
#include <mailutils/cstr.h>

#define SECS_PER_DAY 86400
#define ADJUSTMENT -719162L

/* Julian day is the number of days since Jan 1, 4713 BC (ouch!).
   Eg. Jan 1, 1900 is: */
#define  JULIAN_1900    1721425L

/* Computes the number of days elapsed since January, 1 1900 to the
   January, 1 of the given year */
static unsigned
jan1st (int year)
{
  year--;               /* Do not consider the current year */
  return  year * 365L
               + year/4L    /* Years divisible by 4 are leap years */
               + year/400L  /* Years divisible by 400 are always leap years */
               - year/100L; /* Years divisible by 100 but not 400 aren't */
}

static int  month_start[]=
    {    0, 31, 59,  90, 120, 151, 181, 212, 243, 273, 304, 334, 365 };
     /* Jan Feb Mar  Apr  May  Jun  Jul  Aug  Sep  Oct  Nov  Dec
         31  28  31   30   31   30   31   31   30   31   30   31
     */

/* NOTE: ignore GCC warning. The precedence of operators is OK here */
#define leap_year(y) ((y) % 4 == 0 && (y) % 100 != 0 || (y) % 400 == 0)

static int
dayofyear (int year, int month, int day)
{
  int  leap, month_days;

  if (year < 0 || month < 0 || month > 11)
    return -1;
    
  leap = leap_year (year);
  
  month_days = month_start[month + 1] - month_start[month]
               + ((month == 2) ? leap : 0);

  if (day < 0 || day > month_days)
    return -1;  /* Illegal Date */

  if (month <= 2)
    leap = 0;

  return month_start[month] + day + leap;
}

/* Returns number of days in a given year */
static int
year_days (int year)
{
  return dayofyear (year, 11, 31);
}

static int
julianday (unsigned *pd, int year, int month, int day)
{
  int total = dayofyear (year, month, day);
  if (total == -1)
    return -1;
  *pd = JULIAN_1900 + total + jan1st (year);
  return 0;
}

static int
dayofweek (int year, int month, int day)
{
  unsigned jd;

  if (julianday (&jd, year, month, day))
    return -1;

  /* January 1, 1900 was Monday, hence +1 */
  return (jd + 1) % 7;
}

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

/* Convert struct tm into time_t, taking into account timezone offset. */
/* FIXME: It does not take DST into account */
time_t
mu_tm2time (struct tm *tm, mu_timezone *tz)
{
  time_t t;
  int day;
  
  day = dayofyear (tm->tm_year, tm->tm_mon, tm->tm_mday - 1);
  if (day == -1)
    return -1;
  t = (day + ADJUSTMENT + jan1st (1900 + tm->tm_year)) * SECS_PER_DAY
            + (tm->tm_hour * 60 + tm->tm_min) * 60 + tm->tm_sec
       - (tz ? tz->utc_offset : 0);
  return t;
}

/* Convert time 0 at UTC to our localtime, that tells us the offset
   of our current timezone from UTC. */
time_t
mu_utc_offset (void)
{
  time_t t = 0;
  struct tm *tm = gmtime (&t);

  return - mktime (tm);
}

static const char *short_month[] =
{
  "Jan", "Feb", "Mar", "Apr", "May", "Jun",
  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

static const char *full_month[] =
{
  "January", "February", "March", "April",
  "May", "June", "July", "August",
  "September", "October", "November", "December"
};

static const char *short_wday[] =
{
  "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

static const char *full_wday[] =
{
  "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday",
  "Friday", "Saturday"
};

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
	    rc = mu_stream_write (str, short_wday[tm.tm_wday],
				  strlen (short_wday[tm.tm_wday]), NULL);
	  break;
	  
	case 'A':
	  /* The full weekday name. */
	  if (tm.tm_wday < 0 || tm.tm_wday > 6)
	    rc = ERANGE;
	  else
	    rc = mu_stream_write (str, full_wday[tm.tm_wday],
				  strlen (full_wday[tm.tm_wday]), NULL);
	  break;
	  
	case 'b':
	case 'h':
	  /* The abbreviated month name. */
	  if (tm.tm_mon < 0 || tm.tm_mon > 11)
	    rc = ERANGE;
	  else
	    rc = mu_stream_write (str, short_month[tm.tm_mon],
				  strlen (short_month[tm.tm_mon]), NULL);
	  break;
	  
	case 'B':
	  /* The full month name. */
	  if (tm.tm_mon < 0 || tm.tm_mon > 11)
	    rc = ERANGE;
	  else
	    rc = mu_stream_write (str, full_month[tm.tm_mon],
				  strlen (full_month[tm.tm_mon]), NULL);
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
		days = ISO_8601_weekdays (tm.tm_yday + year_days (year - 1),
					  tm.tm_wday);
		year--;
	      }
	    else
	      {
		int d = ISO_8601_weekdays (tm.tm_yday - year_days (year),
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
				 (unsigned long) mu_tm2time (&tm, tz));
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
	  
	case 'z':
	case 'Z':
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

int
mu_parse_imap_date_time (const char **p, struct tm *tm, mu_timezone *tz)
{
  int year, mon, day, hour, min, sec;
  char zone[6] = "+0000";	/* ( "+" / "-" ) hhmm */
  char month[5] = "";
  int hh = 0;
  int mm = 0;
  int sign = 1;
  int scanned = 0, scanned3;
  int i;
  int tzoffset;

  day = mon = year = hour = min = sec = 0;

  memset (tm, 0, sizeof (*tm));

  switch (sscanf (*p,
		  "%2d-%3s-%4d%n %2d:%2d:%2d %5s%n",
		  &day, month, &year, &scanned3, &hour, &min, &sec, zone,
		  &scanned))
    {
    case 3:
      scanned = scanned3;
      break;
    case 7:
      break;
    default:
      return -1;
    }

  tm->tm_sec = sec;
  tm->tm_min = min;
  tm->tm_hour = hour;
  tm->tm_mday = day;

  for (i = 0; i < 12; i++)
    {
      if (mu_c_strncasecmp (month, short_month[i], 3) == 0)
	{
	  mon = i;
	  break;
	}
    }
  tm->tm_mon = mon;
  tm->tm_year = (year > 1900) ? year - 1900 : year;
  tm->tm_yday = 0;		/* unknown. */
  tm->tm_wday = 0;		/* unknown. */
#if HAVE_STRUCT_TM_TM_ISDST
  tm->tm_isdst = -1;		/* unknown. */
#endif

  hh = (zone[1] - '0') * 10 + (zone[2] - '0');
  mm = (zone[3] - '0') * 10 + (zone[4] - '0');
  sign = (zone[0] == '-') ? -1 : +1;
  tzoffset = sign * (hh * 60 * 60 + mm * 60);

#if HAVE_STRUCT_TM_TM_GMTOFF
  tm->tm_gmtoff = tzoffset;
#endif

  if (tz)
    {
      tz->utc_offset = tzoffset;
      tz->tz_name = NULL;
    }

  *p += scanned;

  return 0;
}

/* "ctime" format is: Thu Jul 01 15:58:27 1999, with no trailing \n.  */
int
mu_parse_ctime_date_time (const char **p, struct tm *tm, mu_timezone *tz)
{
  int wday = 0;
  int year = 0;
  int mon = 0;
  int day = 0;
  int hour = 0;
  int min = 0;
  int sec = 0;
  int n = 0;
  int i;
  char weekday[5] = "";
  char month[5] = "";

  if (sscanf (*p, "%3s %3s %2d %2d:%2d:%2d %d%n\n",
	weekday, month, &day, &hour, &min, &sec, &year, &n) != 7)
    return -1;

  *p += n;

  for (i = 0; i < 7; i++)
    {
      if (mu_c_strncasecmp (weekday, short_wday[i], 3) == 0)
	{
	  wday = i;
	  break;
	}
    }

  for (i = 0; i < 12; i++)
    {
      if (mu_c_strncasecmp (month, short_month[i], 3) == 0)
	{
	  mon = i;
	  break;
	}
    }

  if (tm)
    {
      memset (tm, 0, sizeof (struct tm));

      tm->tm_sec = sec;
      tm->tm_min = min;
      tm->tm_hour = hour;
      tm->tm_mday = day;
      tm->tm_wday = wday;
      tm->tm_mon = mon;
      tm->tm_year = (year > 1900) ? year - 1900 : year;
#ifdef HAVE_STRUCT_TM_TM_ISDST
      tm->tm_isdst = -1;	/* unknown. */
#endif
    }

  /* ctime has no timezone information, set tz to local TZ if they ask. */
  if (tz)
    {
      tz->utc_offset = mu_utc_offset ();
      tz->tz_name = NULL;
    }
  
  return 0;
}
