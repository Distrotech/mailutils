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
#include <mailutils/mailutils.h>
#include <mailutils/libcfg.h>
#include <argp.h>

static char cflags_doc[] = N_("mu cflags - show compiler options");
char cflags_docstring[] = N_("show compiler options");

static struct argp cflags_argp = {
  NULL,
  NULL,
  NULL,
  cflags_doc,
  NULL,
  NULL,
  NULL
};

int
mutool_cflags (int argc, char **argv)
{
  if (argp_parse (&cflags_argp, argc, argv, ARGP_IN_ORDER, NULL, NULL))
    return 1;
  mu_printf ("%s\n", COMPILE_FLAGS);
  return 0;
}

/*
  MU Setup: cflags
  mu-handler: mutool_cflags
  mu-docstring: cflags_docstring
  End MU Setup:
*/

