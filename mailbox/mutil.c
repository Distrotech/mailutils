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

#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#include <time.h>
#include <pwd.h>
#include <unistd.h>
#include <string.h>

#include <mailutils/mutil.h>
#include <mailutils/iterator.h>

/* convert a sequence of hex characters into an integer */

unsigned long
mu_hex2ul (char hex)
{
  if (hex >= '0' && hex <= '9')
    return hex - '0';

  if (hex >= 'a' && hex <= 'z')
    return hex - 'a';

  if (hex >= 'A' && hex <= 'Z')
    return hex - 'A';

  return -1;
}

size_t
mu_hexstr2ul (unsigned long *ul, const char *hex, size_t len)
{
  size_t r;

  *ul = 0;

  for (r = 0; r < len; r++)
    {
      unsigned long v = mu_hex2ul (hex[r]);

      if (v == (unsigned long)-1)
	return r;

      *ul = *ul * 16 + v;
    }
  return r;
}

/* Convert struct tm into time_t, taking into account timezone offset.

 mktime() always treats tm as if it was localtime, so convert it
 to UTC, then adjust by the tm's real timezone, if it is known.

 NOTE: 1. mktime converts localtime struct tm to *time_t in UTC*
       2. adding mu_utc_offset() compensates for the localtime
          corrections in mktime(), i.e. it yields the time_t in
	  the current zone of struct tm.
       3. finally, subtracting TZ offset yields the UTC.
*/
time_t
mu_tm2time (struct tm *timeptr, mu_timezone* tz)
{
  int offset = tz ? tz->utc_offset : 0;

  return mktime (timeptr) + mu_utc_offset () - offset;
}

/* Convert time 0 at UTC to our localtime, that tells us the offset
   of our current timezone from UTC. */
time_t
mu_utc_offset (void)
{
  time_t t = 0;
  struct tm* tm = gmtime (&t);

  return - mktime (tm);
}

static const char *months[] =
{
  "Jan", "Feb", "Mar", "Apr", "May", "Jun",
  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec", NULL
};

static const char *wdays[] =
{
  "Mon", "Teu", "Wed", "Thr", "Fri", "Sat", "Sun", NULL
};

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
      if (strncasecmp (month, months[i], 3) == 0)
	{
	  mon = i;
	  break;
	}
    }
  tm->tm_mon = mon;
  tm->tm_year = (year > 1900) ? year - 1900 : year;
  tm->tm_yday = 0;		/* unknown. */
  tm->tm_wday = 0;		/* unknown. */
#if HAVE_TM_ISDST
  tm->tm_isdst = -1;		/* unknown. */
#endif

  hh = (zone[1] - '0') * 10 + (zone[2] - '0');
  mm = (zone[3] - '0') * 10 + (zone[4] - '0');
  sign = (zone[0] == '-') ? -1 : +1;
  tzoffset = sign * (hh * 60 * 60 + mm * 60);

#if HAVE_TM_GMTOFFSET
  tm->tm_gmtoffset = tzoffset;
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
mu_parse_ctime_date_time (const char **p, struct tm *tm, mu_timezone * tz)
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
      if (strncasecmp (weekday, wdays[i], 3) == 0)
	{
	  wday = i;
	  break;
	}
    }

  for (i = 0; i < 12; i++)
    {
      if (strncasecmp (month, months[i], 3) == 0)
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
#ifdef HAVE_TM_ISDST
      tm->tm_isdst = -1;	/* unknown. */
#endif
    }

  /* ctime has no timezone information, set tz to UTC if they ask. */
  if (tz)
    memset (tz, 0, sizeof (struct mu_timezone));

  return 0;
}

char *
mu_get_homedir (void)
{
  char *homedir = getenv ("HOME");
  if (!homedir)
    {
      struct passwd *pwd;

      pwd = getpwuid (getuid ());
      if (!pwd)
	return NULL;
      homedir = pwd->pw_dir;
    }
  return homedir;
}

/* NOTE: Allocates Memory.  */
/* Expand: ~ --> /home/user and to ~guest --> /home/guest.  */
char *
mu_tilde_expansion (const char *ref, const char *delim, const char *homedir)
{
  char *p = strdup (ref);

  if (*p == '~')
    {
      p++;
      if (*p == delim[0] || *p == '\0')
        {
	  char *s;
	  if (!homedir)
	    {
	      homedir = mu_get_homedir ();
	      if (!homedir)
		return NULL;
	    }
	  s = calloc (strlen (homedir) + strlen (p) + 1, 1);
          strcpy (s, homedir);
          strcat (s, p);
          free (--p);
          p = s;
        }
      else
        {
          struct passwd *pw;
          char *s = p;
          char *name;
          while (*s && *s != delim[0])
            s++;
          name = calloc (s - p + 1, 1);
          memcpy (name, p, s - p);
          name [s - p] = '\0';
          pw = mu_getpwnam (name);
          free (name);
          if (pw)
            {
              char *buf = calloc (strlen (pw->pw_dir) + strlen (s) + 1, 1);
              strcpy (buf, pw->pw_dir);
              strcat (buf, s);
              free (--p);
              p = buf;
            }
          else
            p--;
        }
    }
  return p;
}

/* Smart strncpy that always add the null and returns the number of bytes
   written.  */
size_t
util_cpystr (char *dst, const char *src, size_t size)
{
  size_t len = src ? strlen (src) : 0 ;
  if (dst == NULL || size == 0)
    return len;
  if (len >= size)
    len = size - 1;
  memcpy (dst, src, len);
  dst[len] = '\0';
  return len;
}

static list_t _app_getpwnam = NULL;

void
mu_register_getpwnam (struct passwd *(*fun) __P((const char *)))
{
  if (!_app_getpwnam && list_create (&_app_getpwnam))
    return;
  list_append (_app_getpwnam, fun);
}

struct passwd *
mu_getpwnam (const char *name)
{
  struct passwd *p;
  iterator_t itr;

  p = getpwnam (name);

  if (!p && iterator_create (&itr, _app_getpwnam) == 0)
    {
      struct passwd *(*fun) __P((const char *));
      for (iterator_first (itr); !p && !iterator_is_done (itr);
	   iterator_next (itr))
	{
	  iterator_current (itr, (void **)&fun);
	  p = (*fun) (name);
	}

      iterator_destroy (&itr);
    }
  return p;
}

int mu_virtual_domain;

#ifdef USE_VIRTUAL_DOMAINS

struct passwd *
getpwnam_virtual (const char *u)
{
  struct passwd *pw = NULL;
  FILE *pfile;
  size_t i = 0, len = strlen (u), delim = 0;
  char *filename;

  mu_virtual_domain = 0;
  for (i = 0; i < len && delim == 0; i++)
    if (u[i] == '!' || u[i] == ':' || u[i] == '@')
      delim = i;

  if (delim == 0)
    return NULL;

  filename = malloc (strlen (SITE_VIRTUAL_PWDDIR) +
		     strlen (&u[delim + 1]) + 2 /* slash and null byte */);
  if (filename == NULL)
    return NULL;

  sprintf (filename, "%s/%s", SITE_VIRTUAL_PWDDIR, &u[delim + 1]);
  pfile = fopen (filename, "r");
  free (filename);

  if (pfile)
    while ((pw = fgetpwent (pfile)) != NULL)
      {
	if (strlen (pw->pw_name) == delim && !strncmp (u, pw->pw_name, delim))
	  {
	    mu_virtual_domain = 1;
	    break;
	  }
      }

  return pw;
}

#endif
