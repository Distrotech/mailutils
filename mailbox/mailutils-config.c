/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002 Free Software Foundation, Inc.

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
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <mailutils/mailutils.h>
#include <mailutils/argp.h>

const char *argp_program_version = "mailutils-config (" PACKAGE_STRING ")";
static char doc[] = N_("GNU mailutils-config -- Display compiler and loader options needed for building a program with mailutils");
static char args_doc[] = N_("[arg...]");

static struct argp_option options[] = {
  {"compile", 'c', NULL,   0, N_("print C compiler flags to compile with"), 0},
  {"link",    'l', NULL,   0,
   N_("print libraries to link with. Up to two args can be given. Arguments are: "
   "auth, to display libraries needed for linking against libmuauth, and "
   "guile, to display libraries needed for linking against libmu_scm. "
   "Both can be given simultaneously"), 0},
  {"info", 'i', NULL, 0,
   N_("print a list of compilation options used to build mailutils. If arguments "
   "are given, they are interpreted as a list of compilation options to check "
   "for. In this case the program prints those options from this list that "
   "have been defined. It exits with zero status if all of the "
   "specified options are defined. Otherwise, the exit status is 1."), 0},
  {0, 0, 0, 0}
};

enum config_mode {
  MODE_VOID,
  MODE_COMPILE,
  MODE_LINK,
  MODE_INFO
};

enum config_mode mode;

static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  switch (key)
    {
    case 'l':
      mode = MODE_LINK;
      break;

    case 'c':
      mode = MODE_COMPILE;
      break;

    case 'i':
      mode = MODE_INFO;
      break;
      
    default: 
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

static struct argp argp = {
  options,
  parse_opt,
  args_doc,
  doc,
  NULL,
  NULL, NULL
};

static const char *argp_capa[] = {
  "common",
  "license",
  NULL
};

int
main (int argc, char **argv)
{
  int index;
  
  if (mu_argp_parse (&argp, &argc, &argv, 0, argp_capa, &index, NULL))
    {
      argp_help (&argp, stdout, ARGP_HELP_SEE, program_invocation_short_name);
      return 1;
    }

  argc -= index;
  argv += index;
  
  switch (mode)
    {
    case MODE_VOID:
      break;

    case MODE_LINK:
	{
	  int n = 0, j;
	  struct entry {
	    int level;
	    char *ptr;
	  } entry[4];

	  entry[n].level = 1;
	  asprintf (&entry[n].ptr, "%s -lmailbox", LINK_FLAGS);
	  n++;
#ifdef ENABLE_NLS
	  entry[n].level = 10;
	  asprintf (&entry[n].ptr, "-lintl -liconv");
	  n++;
#endif
	  for (; n < sizeof(entry)/sizeof(entry[0]) && argc > 0;
	       argc--, argv++, n++)
	    {
	      if (strcmp (argv[0], "auth") == 0)
		{
		  entry[n].level = 2;
		  asprintf (&entry[n].ptr, "-lmuauth %s", AUTHLIBS);
		}
#ifdef WITH_GUILE	      
	      else if (strcmp (argv[0], "guile") == 0)
		{
		  entry[n].level = -1;
		  asprintf (&entry[n].ptr, "-lmu_scm %s", GUILE_LIBS);
		}
#endif
	      else
		{
		  argp_help (&argp, stdout, ARGP_HELP_USAGE,
			     program_invocation_short_name);
		  return 1;
		}
	    }

	  /* Sort the entires by their level. */
	  for (j = 0; j < n; j++)
	    {
	      int i;
	      
	      for (i = j; i < n; i++)
		if (entry[j].level > entry[i].level)
		  {
		    struct entry tmp;
		    tmp = entry[i];
		    entry[i] = entry[j];
		    entry[j] = tmp;
		  }
	      
	    }

	  /* At least one entry is always present */
	  printf ("%s", entry[0].ptr);

	  /* Print the rest of them separated by a space */
	  for (j = 1; j < n; j++)
	    {
	      if (entry[j].level == entry[j-1].level)
		continue;
	      printf (" %s", entry[j].ptr);
	    }
	  printf ("\n");
	  return 0;
	}
	
    case MODE_COMPILE:
      if (argc != 0)
	break;
      printf ("%s\n", COMPILE_FLAGS);
      return 0;

    case MODE_INFO:
      if (argc == 0)
	mu_print_options ();
      else
	{
	  int i, found = 0;
	  
	  for (i = 0; i < argc; i++)
	    {
	      const char *val = mu_check_option (argv[i]);
	      if (val)
		{
		  found++;
		  printf ("%s\n", val);
		}
	    }
	  return found == argc ? 0 : 1;
	}
      return 0;
    }
  
  argp_help (&argp, stdout, ARGP_HELP_USAGE, program_invocation_short_name);
  return 0;
}
  

