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

/* MH rmf command */

#include <mh.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include <dirent.h>

const char *argp_program_version = "rmf (" PACKAGE_STRING ")";
static char doc[] = N_("GNU MH rmf\v"
"Use -help to obtain the list of traditional MH options.");
static char args_doc[] = N_("[+folder]");

/* GNU options */
static struct argp_option options[] = {
  {"folder",  ARG_FOLDER, N_("FOLDER"), 0,
   N_("Specify the folder to delete")},
  {"interactive", ARG_INTERACTIVE, N_("BOOL"), OPTION_ARG_OPTIONAL,
    N_("Interactive mode: ask for confirmation before removing each folder")},
  {"recursive", ARG_RECURSIVE, NULL, 0,
   N_("Recursively delete all subfolders")},
  { 0 }
};

/* Traditional MH options */
struct mh_option mh_option[] = {
  {"interactive", 1, MH_OPT_BOOL, NULL},
  { 0 }
};

int explicit_folder; /* Was the folder explicitly given */
int interactive; /* Ask for confirmation before deleting */
int recurse;     /* Recursively process all the sub-directories */

static int
opt_handler (int key, char *arg, void *unused, struct argp_state *state)
{
  switch (key)
    {
    case '+':
    case ARG_FOLDER:
      explicit_folder = 1;
      current_folder = arg;
      break;

    case ARG_INTERACTIVE:
      interactive = is_true (arg);
      break;

    case ARG_RECURSIVE:
      recurse = is_true (arg);
      break;
      
    default:
      return 1;
    }
  return 0;
}

static void
rmf (const char *name)
{
  DIR *dir;
  struct dirent *entry;
    
  dir = opendir (name);

  if (!dir)
    {
      mh_error (_("can't scan folder %s: %s"), name, strerror (errno));
      return;
    }

  if (interactive && !mh_getyn (_("Remove folder %s"), name))
    exit (0);
  
  while ((entry = readdir (dir)))
    {
      char *p;
      struct stat st;

      if (strcmp (entry->d_name, ".") == 0
	  || strcmp (entry->d_name, "..") == 0)
	continue;
      
      asprintf (&p, "%s/%s", name, entry->d_name);
      if (stat (p, &st) < 0)
	{
	  mh_error (_("can't stat %s: %s"), p, strerror (errno));
	}
      else if (S_ISDIR (st.st_mode))
	{
	  if (recurse)
	    rmf (p);
	}
      else
	{
	  if (unlink (p))
	    mh_error (_("can't unlink %s: %s"), p, strerror (errno));
	}
      free (p);
    }
  closedir (dir);
  rmdir (name);
}

int
main (int argc, char **argv)
{
  char *name;

  /* Native Language Support */
  mu_init_nls ();

  mh_argp_parse (argc, argv, 0, options, mh_option, args_doc, doc,
		 opt_handler, NULL, NULL);
  if (!explicit_folder)
    interactive = 1;

  name = mh_expand_name (NULL, current_folder, 0);
  rmf (name);
  return 0;
}
