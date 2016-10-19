/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2010-2012, 2014-2016 Free Software Foundation, Inc.

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

#include "mu.h"

static char info_doc[] = N_("mu info - print a list of configuration\
 options used to build mailutils; optional arguments are interpreted\
 as a list of configuration options to check for.");
char info_docstring[] = N_("show Mailutils configuration");
static char info_args_doc[] = N_("[capa...]");

static int verbose;

static struct mu_option info_options[] = {
  { "verbose", 'v', NULL, MU_OPTION_DEFAULT,
    N_("increase output verbosity"),
    mu_c_bool, &verbose },
  MU_OPTION_END
};

int
mutool_info (int argc, char **argv)
{
  mu_action_getopt (&argc, &argv, info_options, info_doc, info_args_doc);

  if (argc == 0)
    mu_format_options (mu_strout, verbose);
  else
    {
      int i, found = 0;
      
      for (i = 0; i < argc; i++)
	{
	  const struct mu_conf_option *opt = mu_check_option (argv[i]);
	  if (opt)
	    {
	      found++;
	      mu_format_conf_option (mu_strout, opt, verbose);
	    }
	}
      return found == argc ? 0 : 1;
    }
  return 0;
}

/*
  MU Setup: info
  mu-handler: mutool_info
  mu-docstring: info_docstring
  End MU Setup:
*/
