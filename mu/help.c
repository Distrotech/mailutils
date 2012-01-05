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

/* mu help - specjalnie dla Wojtka :) */

#if defined(HAVE_CONFIG_H)
# include <config.h>
#endif
#include <stdlib.h>
#include <mailutils/nls.h>
#include <mailutils/io.h>
#include "mailutils/libargp.h"
#include "mu.h"

static char help_doc[] = N_("mu help - display a terse help summary");
char help_docstring[] = N_("display a terse help summary");
static char help_args_doc[] = N_("[COMMAND]");

static struct argp help_argp = {
  NULL,
  NULL,
  help_args_doc,
  help_doc,
  NULL,
  NULL,
  NULL
};

int
mutool_help (int argc, char **argv)
{
  int index;

  if (argp_parse (&help_argp, argc, argv, ARGP_IN_ORDER, &index, NULL))
    return 1;

  if (index == argc - 1)
    {
      mutool_action_t action = dispatch_find_action (argv[index]);
      if (!action)
	{
	  mu_error (_("don't know what %s is"), argv[index]);
	  exit (1);
	}
      mu_asprintf (&argv[0], "%s %s", mu_program_name, argv[index]);
      argv[1] = "--help";
      return action (2, argv);
    }
  else if (argc != index)
    {
      mu_error (_("too many arguments"));
      exit (1);
    }
  return mu_help ();
}

/*
  MU Setup: help
  mu-handler: mutool_help
  mu-docstring: help_docstring
  End MU Setup:
*/
