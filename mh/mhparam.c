/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2002 Free Software Foundation, Inc.

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

/* MH mhpath command */

#include <mh.h>

const char *program_version = "mhparam (" PACKAGE_STRING ")";
static char doc[] = N_("GNU MH mhparam\v"
"Use -help to obtain the list of traditional MH options.");
static char args_doc[] = N_("[components]");

/* GNU options */
static struct argp_option options[] = {
  {"all",  ARG_ALL, NULL, 0,
   N_("Display all components from the MH profile. All other arguments are ignored")},
  {"component", ARG_COMPONENT, N_("BOOL"),   OPTION_ARG_OPTIONAL,
   N_("Always display the component name") },
  {"license", ARG_LICENSE, 0,      0,
   N_("Display software license"), -1},
  { 0 }
};

/* Traditional MH options */
struct mh_option mh_option[] = {
  { "all", 1, 0 },
  { "component", 1, MH_OPT_BOOL},
  { 0 }
};

static int display_all;
static int display_comp_name = -1;

static int
opt_handler (int key, char *arg, void *unused, struct argp_state *state)
{
  switch (key)
    {
    case ARG_ALL:
      display_all = 1;
      break;
      
    case ARG_COMPONENT:
      display_comp_name = is_true (arg);
      break;
      
    case ARG_LICENSE:
      mh_license (argp_program_version);
      break;

    default:
      return 1;
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
  for (i = 0; i < sizeof(defvaltab)/sizeof(defvaltab[0]); i++)
    if (strcasecmp(defvaltab[i].comp, comp) == 0)
      return defvaltab[i].val;
  return NULL;
}

int
mhparam_iterator (char *comp, char *value, char *data ARG_UNUSED)
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
      char *val = mh_global_profile_get (comp, NULL);
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
  mu_init_nls ();

  mu_argp_init (program_version, NULL);
  mh_argp_parse (argc, argv, 0, options, mh_option, args_doc, doc,
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
  
