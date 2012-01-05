/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003, 2007-2012 Free Software Foundation, Inc.

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

#include <mh.h>

static char doc[] = N_("GNU MH install-mh")"\v"
N_("Use -help to obtain the list of traditional MH options.");

/* GNU options */
static struct argp_option options[] = {
  {"auto",  ARG_AUTO, NULL, 0, N_("do not ask for anything")},
  {"compat", ARG_COMPAT, NULL, OPTION_HIDDEN, ""},
  {NULL}
};

struct mh_option mh_option[] = {
  { "auto" },
  { "compat" },
  { NULL }
};

int automode;

static error_t
opt_handler (int key, char *arg, struct argp_state *state)
{
  switch (key)
    {
    case ARG_AUTO:
      automode = 1;
      break;

    case ARG_COMPAT:
      break;

    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

int
main (int argc, char **argv)
{
  char *name;
  extern int mh_auto_install;
  
  /* Native Language Support */
  MU_APP_INIT_NLS ();

  mh_argp_init ();
  mh_auto_install = 0;
  mh_argp_parse (&argc, &argv, 0, options, mh_option, NULL, doc,
		 opt_handler, NULL, NULL);

  name = getenv ("MH");
  if (name)
    name = mu_tilde_expansion (name, MU_HIERARCHY_DELIMITER, NULL);
  else
    {
      char *home = mu_get_homedir ();
      if (!home)
	abort (); /* shouldn't happen */
      name = mh_safe_make_file_name (home, MH_USER_PROFILE);
      free (home);
    }
  mh_install (name, automode);
  return 0;
}
  


  
