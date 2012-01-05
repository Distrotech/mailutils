/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2011-2012 Free Software Foundation, Inc.

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
#include <mailutils/stream.h>
#include <mailutils/stdstream.h>
#include <mailutils/diag.h>

int
main (int argc, char **argv)
{
  mu_stream_t str = mu_strout;
  int i;
  
  if (argc == 1)
    {
      fprintf (stderr, "usage: %s: word|option [word|option...]\n", argv[0]);
      fprintf (stderr, "options are: -out, -err, -reset\n");
      return 1;
    }

  mu_set_program_name (argv[0]);
  
  for (i = 1; i < argc; i++)
    {
      char *arg = argv[i];
      if (arg[0] == '-')
	{
	  if (strcmp (arg, "-out") == 0)
	    str = mu_strout;
	  else if (strcmp (arg, "-err") == 0)
	    str = mu_strerr;
	  else if (strcmp (arg, "-reset") == 0)
	    {
	      if (str == mu_strout)
		{
		  mu_stdstream_setup (MU_STDSTREAM_RESET_STROUT);
		  str = mu_strout;
		}
	      else
		{
		  mu_stdstream_setup (MU_STDSTREAM_RESET_STRERR);
		  str = mu_strerr;
		}
	    }
	  else
	    {
	      fprintf (stderr, "%s: unrecognized option %s\n", argv[0], arg);
	      return 1;
	    }
	}
      else
	mu_stream_printf (str, "%s\n", arg);
    }
  
  return 0;
}
