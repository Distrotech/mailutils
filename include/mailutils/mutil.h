/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Library Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifndef _MAILUTILS_MUTIL_H
#define _MAILUTILS_MUTIL_H

/*
   Collection of useful utility routines that are worth sharing,
   but don't have a natural home somewhere else.
*/

#include <time.h>

#ifndef __P
# ifdef __STDC__
#  define __P(args) args
# else
#  define __P(args) ()
# endif
#endif /*__P */

#ifdef __cplusplus
extern "C" {
#endif

struct mu_timezone
{
  int utc_offset;
    /* Seconds east of UTC. */

  const char *tz_name;
    /* Nickname for this timezone, if known. It is always considered
     * to be a pointer to static string, so will never be freed. */
};

typedef struct mu_timezone mu_timezone;

extern int mu_parse_imap_date_time __P ((const char **p, struct tm * tm,
					 mu_timezone * tz));
extern int mu_parse_ctime_date_time __P ((const char **p, struct tm * tm,
					 mu_timezone * tz));

extern time_t mu_utc_offset __P ((void));
extern time_t mu_tm2time __P ((struct tm * timeptr, mu_timezone * tz));

#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_MUTIL_H */

