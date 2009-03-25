/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002, 2007, 2009
   Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
   MA 02110-1301 USA */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <string.h>
#include <mailutils/mailutils.h>
#include <mu_asprintf.h>
#include "mailutils/libargp.h"

const char *program_version = "mailutils-config (" PACKAGE_STRING ")";
static char doc[] = N_("GNU mailutils-config -- Display compiler and loader options needed for building a program with mailutils");
static char args_doc[] = N_("[arg...]");

static struct argp_option options[] = {
  {"compile", 'c', NULL,   0,
   N_("Print C compiler flags to compile with"), 0},
  {"link",    'l', NULL,   0,
   N_("Print libraries to link with. Possible arguments are: auth, guile, "
      "mbox, mh, maildir, mailer, imap, pop, sieve and all"), 0},
  {"info", 'i', NULL, 0,
   N_("Print a list of configuration options used to build mailutils. If arguments "
   "are given, they are interpreted as a list of configuration options to check "
   "for. In this case the program prints those options from this list that "
   "have been defined. It exits with zero status if all of the "
   "specified options are defined. Otherwise, the exit status is 1."), 0},
  {"verbose", 'v', NULL, 0,
   N_("Increase output verbosity"), 0},
  {0, 0, 0, 0}
};

enum config_mode {
  MODE_VOID,
  MODE_COMPILE,
  MODE_LINK,
  MODE_INFO
};

enum config_mode mode;
int verbose;

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

    case 'v':
      verbose++;
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

#ifdef WITH_TLS
# define NEEDAUTH 1
#else
# define NEEDAUTH 0
#endif
#define NOTALL   2

struct lib_descr {
  char *name;
  char *libname;
  int flags;
} lib_descr[] = {
  { "mbox",   "mu_mbox", 0 },
  { "mh",     "mu_mh",   0 },
  { "maildir","mu_maildir", 0 },
  { "imap",   "mu_imap", NEEDAUTH },
  { "pop",    "mu_pop",  NEEDAUTH },
  { "nntp",   "mu_nntp", 0 },
  { "mailer", "mu_mailer", 0 },
  { "sieve",  "mu_sieve", NOTALL },
  { NULL }
};

struct lib_entry {
  int level;
  char *ptr;
} lib_entry[16];

int nentry;

void
add_entry (int level, char *ptr)
{
  int i;
  if (nentry >= sizeof(lib_entry)/sizeof(lib_entry[0]))
    {
      mu_error (_("Too many arguments"));
      exit (1);
    }
  
  for (i = 0; i < nentry; i++)
    if (strcmp (lib_entry[i].ptr, ptr) == 0)
      return;
  lib_entry[nentry].level = level;
  lib_entry[nentry].ptr = ptr;
  nentry++;
}

/* Sort the entries by their level. */
void
sort_entries ()
{
  int j;

  for (j = 0; j < nentry; j++)
    {
      int i;
	      
      for (i = j; i < nentry; i++)
	if (lib_entry[j].level > lib_entry[i].level)
	  {
	    struct lib_entry tmp;
	    tmp = lib_entry[i];
	    lib_entry[i] = lib_entry[j];
	    lib_entry[j] = tmp;
	  }
      
    }
}

int
main (int argc, char **argv)
{
  int index;
  
  mu_argp_init (program_version, NULL);
  if (mu_app_init (&argp, argp_capa, NULL, argc, argv, 0, &index, NULL))
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
	  int j;
	  char *ptr;
	  
	  add_entry (-100, LINK_FLAGS);
	  add_entry (100, LINK_POSTFLAGS);
	  add_entry (1, "-lmailutils");
#ifdef ENABLE_NLS
	  if (sizeof (I18NLIBS) > 1)
	    add_entry (10, I18NLIBS);
#endif

	  for ( ; argc > 0; argc--, argv++)
	    {
	      if (strcmp (argv[0], "auth") == 0)
		{
		  add_entry (2, "-lmu_auth " AUTHLIBS);
		}
#ifdef WITH_GUILE	      
	      else if (strcmp (argv[0], "guile") == 0)
		{
		  add_entry (-1, "-lmu_scm " GUILE_LIBS);
		}
#endif
#ifdef WITH_PYTHON
	      else if (strcmp (argv[0], "python") == 0)
		{
		  add_entry (-1, "-lmu_py " PYTHON_LIBS);
		}
#endif
	      else if (strcmp (argv[0], "cfg") == 0)
		add_entry (-1, "-lmu_cfg");
	      else if (strcmp (argv[0], "argp") == 0)
		add_entry (-2, "-lmu_argp");
	      else if (strcmp (argv[0], "all") == 0)
		{
		  struct lib_descr *p;
		  
		  for (p = lib_descr; p->name; p++)
		    {
		      if (p->flags & NOTALL)
			continue;
		      asprintf (&ptr, "-l%s", p->libname);
		      add_entry (0, ptr);
		      if (p->flags & NEEDAUTH)
			add_entry (2, "-lmu_auth " AUTHLIBS);
		    }
		}
	      else
		{
		  struct lib_descr *p;
		  
		  for (p = lib_descr; p->name; p++)
		    if (strcasecmp (p->name, argv[0]) == 0)
		      break;

		  if (p->name)
		    {
		      asprintf (&ptr, "-l%s", p->libname);
		      add_entry (0, ptr);
		      if (p->flags & NEEDAUTH)
			add_entry (2, "-lmu_auth " AUTHLIBS);
		    }
		  else
		    {
		      argp_help (&argp, stdout, ARGP_HELP_USAGE,
				 program_invocation_short_name);
		      return 1;
		    }
		}
	    }
	  
	  sort_entries ();
	  
	  /* At least one entry is always present */
	  printf ("%s", lib_entry[0].ptr);

	  /* Print the rest of them separated by a space */
	  for (j = 1; j < nentry; j++)
	    {
	      printf (" %s", lib_entry[j].ptr);
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
	mu_fprint_options (stdout, verbose);
      else
	{
	  int i, found = 0;
	  
	  for (i = 0; i < argc; i++)
	    {
	      const struct mu_conf_option *opt = mu_check_option (argv[i]);
	      if (opt)
		{
		  found++;
		  mu_fprint_conf_option (stdout, opt, verbose);
		}
	    }
	  return found == argc ? 0 : 1;
	}
      return 0;
    }
  
  argp_help (&argp, stdout, ARGP_HELP_USAGE, program_invocation_short_name);
  return 0;
}
  
