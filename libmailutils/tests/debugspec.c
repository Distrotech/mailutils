/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2002, 2005, 2007, 2010-2012 Free Software
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
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <mailutils/mailutils.h>

int
main (int argc, char **argv)
{
  char *names = NULL;
  int showunset = 0;
  char *arg;
  
  mu_set_program_name (argv[0]);
  mu_stdstream_setup (MU_STDSTREAM_RESET_NONE);

  if (argc == 1)
    {
      mu_printf ("usage: %s spec\n", argv[0]);
      return 0;
    }

  while (argc--)
    {
      arg = *++argv;

      if (strncmp (arg, "-names=", 7) == 0)
	names = arg + 7;
      else if (strcmp (arg, "-showunset") == 0)
	showunset = 1;
      else if (arg[0] == '-')
	{
	  if (arg[1] == '-' && arg[2] == 0)
	    {
	      argc--;
	      argv++;
	      break;
	    }
	  mu_error ("unrecognised argument: %s", arg);
	  return 1;
	}
      else
	break;
    }

  if (argc != 1)
    {
      mu_error ("usage: %s spec", mu_program_name);
      return 1;
    }
  
  mu_debug_parse_spec (arg);
  
  mu_debug_format_spec (mu_strout, names, showunset);
  mu_printf ("\n");
  
  return 0;
}

    
