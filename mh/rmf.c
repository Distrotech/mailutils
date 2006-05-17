/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2002, 2005, 2006 Free Software Foundation, Inc.

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
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
   MA 02110-1301 USA */

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

const char *program_version = "rmf (" PACKAGE_STRING ")";
/* TRANSLATORS: Please, preserve the vertical tabulation (^K character)
   in this message */
static char doc[] = N_("GNU MH rmf\v\
Use -help to obtain the list of traditional MH options.");
static char args_doc[] = N_("[+folder]");

/* GNU options */
static struct argp_option options[] = {
  {"folder",  ARG_FOLDER, N_("FOLDER"), 0,
   N_("Specify the folder to delete")},
  {"interactive", ARG_INTERACTIVE, N_("BOOL"), OPTION_ARG_OPTIONAL,
    N_("Interactive mode: ask for confirmation before removing each folder")},
  {"nointeractive", ARG_NOINTERACTIVE, NULL, OPTION_HIDDEN, ""},
  {"recursive", ARG_RECURSIVE, NULL, 0,
   N_("Recursively delete all subfolders")},
  {"norecursive", ARG_NORECURSIVE, NULL, OPTION_HIDDEN, ""},
  {"license", ARG_LICENSE, 0,      0,
   N_("Display software license"), -1},
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

static char *cur_folder_path; /* Full pathname of the current folder */
static char *folder_name;     /* Name of the (topmost) folder to be
				 deleted */

static int
opt_handler (int key, char *arg, void *unused, struct argp_state *state)
{
  switch (key)
    {
    case ARG_FOLDER:
      explicit_folder = 1;
      folder_name = arg;
      break;

    case ARG_INTERACTIVE:
      interactive = is_true (arg);
      break;

    case ARG_NOINTERACTIVE:
      interactive = 0;
      break;
	
    case ARG_RECURSIVE:
      recurse = is_true (arg);
      break;
      
    case ARG_NORECURSIVE:
      recurse = 0;
      break;

    case ARG_LICENSE:
      mh_license (argp_program_version);
      break;

    default:
      return 1;
    }
  return 0;
}

static char *
current_folder_path ()
{
  mu_mailbox_t mbox = mh_open_folder (mh_current_folder (), 0);
  mu_url_t url;
  char *p;
  mu_mailbox_get_url (mbox, &url);
  p = (char*) mu_url_to_string (url);
  if (strncmp (p, "mh:", 3) == 0)
    p += 3;
  return p;
}

static int
rmf (const char *name)
{
  DIR *dir;
  struct dirent *entry;
  int failures = 0;
  
  dir = opendir (name);

  if (!dir)
    {
      mu_error (_("Cannot scan folder %s: %s"), name, strerror (errno));
      return 1;
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
	  mu_error (_("Cannot stat %s: %s"), p, strerror (errno));
	}
      else if (S_ISDIR (st.st_mode))
	{
	  if (recurse)
	    failures += rmf (p);
	  else
	    {
	      printf ("%s: file `%s' not deleted, continuing...\n",
		      program_invocation_short_name, p);
	      failures++;
	    }
	}
      else
	{
	  if (unlink (p))
	    {
	      mu_error (_("Cannot unlink %s: %s"), p, strerror (errno));
	      failures++;
	    }
	}
      free (p);
    }
  closedir (dir);

  if (failures == 0)
    failures += rmdir (name);
  else
    printf ("%s: folder `%s' not removed\n",
	    program_invocation_short_name, name);

  if (failures == 0)
    {
      if (cur_folder_path && strcmp (name, cur_folder_path) == 0)
	{
	  current_folder = "inbox";
	  mh_global_sequences_drop ();
	  mh_global_save_state ();
	  printf ("[+inbox now current]\n");
	}
    }
  return failures;
}

int
main (int argc, char **argv)
{
  char *name;

  /* Native Language Support */
  mu_init_nls ();

  mu_argp_init (program_version, NULL);
  mh_argp_parse (&argc, &argv, 0, options, mh_option, args_doc, doc,
		 opt_handler, NULL, NULL);
  if (!explicit_folder)
    interactive = 1;

  cur_folder_path = current_folder_path ();

  name = mh_expand_name (NULL, folder_name, 0);
  rmf (name);
  return 0;
}
