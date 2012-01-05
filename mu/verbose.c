/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2010-2012 Free Software Foundation, Inc.

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

#if defined(HAVE_CONFIG_H)
# include <config.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <mailutils/mailutils.h>
#include "mu.h"

/* Manipulations with verbosity flags, which are common for pop and imap. */
int shell_verbose_flags;

static int
string_to_xlev (const char *name, int *pv)
{
  if (strcmp (name, "secure") == 0)
    *pv = MU_XSCRIPT_SECURE;
  else if (strcmp (name, "payload") == 0)
    *pv = MU_XSCRIPT_PAYLOAD;
  else
    return 1;
  return 0;
}

static int
change_verbose_mask (int set, int argc, char **argv)
{
  int i;
  
  for (i = 0; i < argc; i++)
    {
      int lev;
      
      if (string_to_xlev (argv[i], &lev))
	{
	  mu_error ("unknown level: %s", argv[i]);
	  return 1;
	}
      if (set)
	SET_VERBOSE_MASK (lev);
      else
	CLR_VERBOSE_MASK (lev);
    }
  return 0;
}

int
shell_verbose (int argc, char **argv,
	       void (*set_verbose) (void), void (*set_mask) (void))
{
  if (argc == 1)
    {
      if (QRY_VERBOSE ())
	{
	  mu_printf ("verbose is on");
	  if (HAS_VERBOSE_MASK ())
	    {
	      char *delim = " (";
	    
	      if (QRY_VERBOSE_MASK (MU_XSCRIPT_SECURE))
		{
		  mu_printf ("%ssecure", delim);
		  delim = ", ";
		}
	      if (QRY_VERBOSE_MASK (MU_XSCRIPT_PAYLOAD))
		mu_printf ("%spayload", delim);
	      mu_printf (")");
	    }
	  mu_printf ("\n");
	}
      else
	mu_printf ("verbose is off\n");
    }
  else
    {
      int bv;

      if (get_bool (argv[1], &bv) == 0)
	{
	  if (bv)
	    SET_VERBOSE ();
	  else
	    CLR_VERBOSE ();
	  if (argc > 2)
	    change_verbose_mask (shell_verbose_flags, argc - 2, argv + 2);
	  set_verbose ();
	}
      else if (strcmp (argv[1], "mask") == 0)
	change_verbose_mask (1, argc - 2, argv + 2);
      else if (strcmp (argv[1], "unmask") == 0)
	change_verbose_mask (0, argc - 2, argv + 2);
      else
	mu_error ("unknown subcommand");
      set_mask ();
    }

  return 0;
}

