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

/* MH mhparam command */

#include <mh.h>

static char prog_doc[] = N_("GNU MH mhparam");
static char args_doc[] = N_("[COMPONENT [COMPONENT...]]");

static int display_all;
static int display_comp_name = -1;

static struct mu_option options[] = {
  { "all",  0, NULL, MU_OPTION_DEFAULT,
    N_("display all components from the MH profile. All other arguments are ignored"),
    mu_c_bool, &display_all },
  { "component", 0, NULL, MU_OPTION_DEFAULT,
    N_("always display the component name"),
    mu_c_bool, &display_comp_name },
  MU_OPTION_END
};

static struct {
  char *comp;
  char *val;
} defvaltab[] = {
  { "etcdir", MHLIBDIR },
  { "libdir", MHLIBDIR },
  { "bindir", MHBINDIR },
};

char *
mhparam_defval (char *comp)
{
  int i;
  for (i = 0; i < sizeof (defvaltab) / sizeof (defvaltab[0]); i++)
    if (mu_c_strcasecmp (defvaltab[i].comp, comp) == 0)
      return defvaltab[i].val;
  return NULL;
}

int
mhparam_iterator (const char *comp, const char *value,
		  void *data MU_ARG_UNUSED)
{
  if (display_comp_name)
    printf("%s:\t", comp);
  printf("%s\n", value);
  return 0;
}

void
mhparam (char *comp)
{
  if (comp)
    {
      const char *val = mh_global_profile_get (comp, NULL);
      if (!val)
	val = mhparam_defval (comp);
      if (!val)
	return;
      if (display_comp_name)
	printf("%s:\t", comp);
      printf("%s\n", val);
    }
  else
    {
      mh_global_profile_iterate (mhparam_iterator, NULL);
    }
}

int
main (int argc, char **argv)
{
  /* Native Language Support */
  MU_APP_INIT_NLS ();

  mh_getopt (&argc, &argv, options, 0, args_doc, prog_doc, NULL);

  if (display_all)
    {
      if (display_comp_name == -1)
	display_comp_name = 1;
      mhparam (NULL);
    }
  else
    {
      int i;
      
      if (display_comp_name == -1)
	display_comp_name = argc > 1;
	
      for (i = 0; i < argc; i++)
	mhparam (argv[i]);
    }
  return 0;
}
  
