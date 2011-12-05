/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2007, 2009, 2010, 2011 Free Software
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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <mailutils/error.h>
#include <mailutils/errno.h>
#include <mailutils/util.h>
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
  char *format = "%d-%b-%Y%? %H:%M:%S %z";
  char *buf = NULL;
  size_t size = 0;
  size_t n;

  mu_set_program_name (argv[0]);
  
  mu_stdstream_setup (MU_STDSTREAM_RESET_NONE);

  for (i = 1; i < argc; i++)
    {
      char *opt = argv[i];

      if (strncmp (opt, "-format=", 8) == 0)
	format = opt + 8;
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
      char *endp;
      struct tm tm;
      struct mu_timezone tz;

      mu_rtrim_class (buf, MU_CTYPE_ENDLN);

      rc = mu_scan_datetime (buf, format, &tm, &tz, &endp);
      if (rc)
	{
	  if (*endp)
	    mu_error ("parse failed near %s", endp);
	  else
	    mu_error ("parse failed at end of input");
	  continue;
	}
      if (*endp)
	mu_printf ("# stopped at %s\n", endp);
      /* FIXME: add tm_yday? */
      mu_printf ("sec=%d,min=%d,hour=%d,mday=%d,mon=%d,year=%d,wday=%d,tz=%d\n",
		 tm.tm_sec, tm.tm_min, tm.tm_hour, tm.tm_mday, tm.tm_mon,
		 tm.tm_year, tm.tm_wday, tz.utc_offset);
		 
      //mu_c_streamftime (mu_strout, "%c %z%n", &tm, &tz);
    }
  
  if (rc)
    {
      mu_error ("%s", mu_strerror (rc));
      return 1;
    }
  return 0;
}
