/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <mailutils/mailutils.h>
#include <mailutils/argp.h>

const char *argp_program_version = "mailutils-config (" PACKAGE_STRING ")";
static char doc[] = "GNU mailutils-config";
static char args_doc[] = "[arg...]";

static struct argp_option options[] = {
  {"link",    'l', NULL,   0, "print libraries to link with", 1},
  {NULL,        0, NULL,   0,
   "Up to two args can be given. Each arg is "
   "a string \"auth\" or \"guile\"."
   , 2},
  {"compile", 'c', NULL,   0, "print C compiler flags to compile with", 3},
  {0, 0, 0, 0}
};

enum config_mode {
  MODE_VOID,
  MODE_COMPILE,
  MODE_LINK
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
      argp_help (&argp, stdout, ARGP_HELP_SEE,
		 program_invocation_short_name);
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
	  asprintf (&entry[n].ptr, " %s -lmailbox", LINK_FLAGS);
	  n++;
	  
	  for (; n < sizeof(entry)/sizeof(entry[0]) && argc > 0;
	       argc--, argv++, n++)
	    {
	      if (strcmp (argv[0], "auth") == 0)
		{
		  entry[n].level = 2;
		  asprintf (&entry[n].ptr, " -lmuauth %s", AUTHLIBS);
		}
#ifdef WITH_GUILE	      
	      else if (strcmp (argv[0], "guile") == 0)
		{
		  entry[n].level = -1;
		  asprintf (&entry[n].ptr, " -lmu_scm %s", GUILE_LIBS);
		}
#endif
	      else
		{
		  argp_help (&argp, stdout, ARGP_HELP_USAGE,
			     program_invocation_short_name);
		  return 1;
		}
	    }

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

	  for (j = 0; j < n; j++)
	    printf ("%s", entry[j].ptr);
	  printf ("\n");
	  return 0;
	}
	
    case MODE_COMPILE:
      if (argc != 0)
	break;
      printf ("%s\n", COMPILE_FLAGS);
      return 0;
    }
  
  argp_help (&argp, stdout, ARGP_HELP_USAGE,
	     program_invocation_short_name);
  return 0;
}
  

