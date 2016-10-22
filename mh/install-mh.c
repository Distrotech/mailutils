/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003, 2007-2012, 2014-2016 Free Software Foundation,
   Inc.

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

static char prog_doc[] = N_("GNU MH install-mh");

int automode;

static struct mu_option options[] = {
  { "auto",   0, NULL, MU_OPTION_DEFAULT,
    N_("do not ask for anything"),
    mu_c_bool, &automode },
  { "compat", 0, NULL, MU_OPTION_HIDDEN, "", mu_c_void },
  MU_OPTION_END
};

int
main (int argc, char **argv)
{
  char *name;
  extern int mh_auto_install;
  
  mh_auto_install = 0;
  mh_getopt (&argc, &argv, options, 0, NULL, prog_doc, NULL);

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
  


  
