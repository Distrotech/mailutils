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

/* This file is a template for writing modules for the `mu' utility.
   It defines an imaginary module FOO, which does nothing.

   Usage checklist:

   1. [ ] Copy this file to another location.
   2. [ ] Replace FOO with the desired module name.
   3. [ ] Edit the text strings marked with `#warning', removing the warnings
          when ready.
   4. [ ] Implement the desired functionality.
   5. [ ] Add the module to Makefile.am
   6. [ ] Remove this comment.
*/

#if defined(HAVE_CONFIG_H)
# include <config.h>
#endif
#include <mailutils/mailutils.h>
#include "argp.h"
#include "mu.h"

#warning "Replace DESCRIPTION with a short description of this module."
static char FOO_doc[] = N_("mu FOO - DESCRIPTION");

#warning "Usually DESCRIPTION is the same text as the one used in FOO_doc."
char FOO_docstring[] = N_("DESCRIPTION");

#warning "Edit ARGDOC or remove this variable if module does not take arguments"
static char FOO_args_doc[] = N_("ARGDOC");

static struct argp_option FOO_options[] = {
  { NULL }
};

static error_t
FOO_parse_opt (int key, char *arg, struct argp_state *state)
{
  switch (key)
    {
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

static struct argp FOO_argp = {
  FOO_options,
  FOO_parse_opt,
  FOO_args_doc,
  FOO_doc,
  NULL,
  NULL,
  NULL
};

int
mutool_FOO (int argc, char **argv)
{
  int index;
  
  if (argp_parse (&FOO_argp, argc, argv, ARGP_IN_ORDER, &index, NULL))
    return 1;
#warning "Add the necessary functionality here"  
  return 0;
}

/*
  MU Setup: FOO
  mu-handler: mutool_FOO
  mu-docstring: FOO_docstring
  End MU Setup:
*/

  
  
