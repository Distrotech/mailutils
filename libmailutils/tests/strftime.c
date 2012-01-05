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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <mailutils/error.h>
#include <mailutils/errno.h>
#include <mailutils/datetime.h>
#include <mailutils/stream.h>
#include <mailutils/cctype.h>
#include <mailutils/cstr.h>
#include <mailutils/stdstream.h>

void
usage ()
{
  mu_stream_printf (mu_strout, "usage: %s [-format=FMT] [-tz=TZ]\n",
		    mu_program_name);
  exit (0);
}

int
main (int argc, char **argv)
{
  int rc, i;
  char *format = "%c";
  char *buf = NULL;
  size_t size = 0;
  size_t n;
  struct mu_timezone tz, *tzp = NULL;

  mu_set_program_name (argv[0]);
  
  mu_set_program_name (argv[0]);
  mu_stdstream_setup (MU_STDSTREAM_RESET_NONE);

  memset (&tz, 0, sizeof tz);
  for (i = 1; i < argc; i++)
    {
      char *opt = argv[i];

      if (strncmp (opt, "-format=", 8) == 0)
	format = opt + 8;
      else if (strncmp (opt, "-tz=", 4) == 0)
	{
	  int sign;
	  int n = atoi (opt + 4);
	  if (n < 0)
	    {
	      sign = -1;
	      n = - n;
	    }
	  else
	    sign = 1;
	  tz.utc_offset = sign * ((n / 100 * 60) + n % 100) * 60;
	  tzp = &tz;
	}
      else if (strcmp (opt, "-h") == 0)
	usage ();
      else
	{
	  mu_error ("%s: unrecognized argument", opt);
	  exit (1);
	}
    }

  while ((rc = mu_stream_getline (mu_strin, &buf, &size, &n)) == 0 && n > 0)
    {
      char *p;
      struct tm *tm;
      time_t t;

      mu_rtrim_class (buf, MU_CTYPE_ENDLN);

      if (*buf == ';')
	{
	  mu_printf ("%s\n", buf);
	  continue;
	}
      t = strtoul (buf, &p, 10);
      if (*p)
	{
	  mu_error ("bad input line near %s", p);
	  continue;
	}

      tm = gmtime (&t);
      mu_c_streamftime (mu_strout, format, tm, tzp);
      mu_printf ("\n");
    }
  
  if (rc)
    {
      mu_error ("%s", mu_strerror (rc));
      return 1;
    }
  return 0;
}
