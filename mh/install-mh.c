/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA  */

#include <mh.h>

const char *argp_program_version = "install-mh (" PACKAGE_STRING ")";
static char doc[] = N_("GNU MH install-mh\v"
"Use -help to obtain the list of traditional MH options.");
static char args_doc[] = "";

/* GNU options */
static struct argp_option options[] = {
  {"auto",  ARG_AUTO, NULL, 0, N_("Do not ask for anything")},
  {"compat", ARG_COMPAT, NULL, OPTION_HIDDEN, ""},
  {NULL}
};

struct mh_option mh_option[] = {
  {"auto",     1, 0, },
  {"compat",     1, 0, },
  { NULL }
};

int automode;

static int
opt_handler (int key, char *arg, void *unused)
{
  switch (key)
    {
    case ARG_AUTO:
      automode = 1;
      break;

    case ARG_COMPAT:
      break;

    default:
      return 1;
    }
  return 0;
}

int
main (int argc, char **argv)
{
  char *home, *name;
  extern int mh_auto_install;
  /* Native Language Support */
  mu_init_nls ();

  mh_auto_install = 0;
  mh_argp_parse (argc, argv, options, mh_option, args_doc, doc,
		 opt_handler, NULL, NULL);

  home = mu_get_homedir ();
  if (!home)
    abort (); /* shouldn't happen */
  asprintf (&name, "%s/%s", home, MH_USER_PROFILE);
  
  mh_install (name, automode);
  return 0;
}
  


  
