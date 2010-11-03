/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2010 Free Software Foundation, Inc.

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

#if defined(HAVE_CONFIG_H)
# include <config.h>
#endif
#include <stdlib.h>
#include <mailutils/mailutils.h>
#include <mailutils/libcfg.h>
#include <argp.h>

static char ldflags_doc[] = N_("mu ldflags - list libraries required to link");
char ldflags_docstring[] = N_("list libraries required to link");
static char ldflags_args_doc[] = N_("KEYWORD [KEYWORD...]");

static struct argp ldflags_argp = {
  NULL,
  NULL,
  ldflags_args_doc,
  ldflags_doc,
  NULL,
  NULL,
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
      mu_error (_("too many arguments"));
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
mutool_ldflags (int argc, char **argv)
{
  int i, j;
  char *ptr;
  
  if (argp_parse (&ldflags_argp, argc, argv, ARGP_IN_ORDER, &i, NULL))
    return 1;

  argc -= i;
  argv += i;
  
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
	add_entry (2, "-lmu_auth " AUTHLIBS);
#ifdef WITH_GUILE	      
      else if (strcmp (argv[0], "guile") == 0)
	add_entry (-1, "-lmu_scm " GUILE_LIBS);
#endif
#ifdef WITH_PYTHON
      else if (strcmp (argv[0], "python") == 0)
	add_entry (-1, "-lmu_py " PYTHON_LIBS);
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
	      mu_asprintf (&ptr, "-l%s", p->libname);
	      add_entry (0, ptr);
	      if (p->flags & NEEDAUTH)
		add_entry (2, "-lmu_auth " AUTHLIBS);
	    }
	}
      else
	{
	  struct lib_descr *p;
	  
	  for (p = lib_descr; p->name; p++)
	    if (mu_c_strcasecmp (p->name, argv[0]) == 0)
	      break;

	  if (p->name)
	    {
	      mu_asprintf (&ptr, "-l%s", p->libname);
	      add_entry (0, ptr);
	      if (p->flags & NEEDAUTH)
		add_entry (2, "-lmu_auth " AUTHLIBS);
	    }
	  else
	    {
	      mu_error (_("unknown keyword: %s"), argv[0]);
	      return 1;
	    }
	}
    }
  
  sort_entries ();
	  
  /* At least one entry is always present */
  printf ("%s", lib_entry[0].ptr);

  /* Print the rest of them separated by a space */
  for (j = 1; j < nentry; j++)
    printf (" %s", lib_entry[j].ptr);
  putchar ('\n');
  return 0;
}

/*
  MU Setup: ldflags
  mu-handler: mutool_ldflags
  mu-docstring: ldflags_docstring
  End MU Setup:
*/
