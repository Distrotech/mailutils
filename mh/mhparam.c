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

/* MH mhparam command */

#include <mh.h>

static char doc[] = N_("GNU MH mhparam")"\v"
N_("Use -help to obtain the list of traditional MH options.");
static char args_doc[] = N_("[COMPONENT [COMPONENT...]]");

/* GNU options */
static struct argp_option options[] = {
  {"all",  ARG_ALL, NULL, 0,
   N_("display all components from the MH profile. All other arguments are ignored")},
  {"component", ARG_COMPONENT, N_("BOOL"),   OPTION_ARG_OPTIONAL,
   N_("always display the component name") },
  { 0 }
};

/* Traditional MH options */
struct mh_option mh_option[] = {
  { "all" },
  { "component", MH_OPT_BOOL },
  { NULL }
};

static int display_all;
static int display_comp_name = -1;

static error_t
opt_handler (int key, char *arg, struct argp_state *state)
{
  switch (key)
    {
    case ARG_ALL:
      display_all = 1;
      break;
      
    case ARG_COMPONENT:
      display_comp_name = is_true (arg);
      break;
      
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

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
mhparam_iterator (const char *comp, const char *value, void *data MU_ARG_UNUSED)
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
  int index;
  
  /* Native Language Support */
  MU_APP_INIT_NLS ();

  mh_argp_init ();
  mh_argp_parse (&argc, &argv, 0, options, mh_option, args_doc, doc,
		 opt_handler, NULL, &index);

  if (display_all)
    {
      if (display_comp_name == -1)
	display_comp_name = 1;
      mhparam (NULL);
    }
  else
    {
      if (display_comp_name == -1)
	display_comp_name = argc - index > 1;
	
      for (; index < argc; index++)
	mhparam (argv[index]);
    }
  return 0;
}
  
