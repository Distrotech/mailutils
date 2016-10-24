/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2002, 2005-2012, 2014-2016 Free Software Foundation,
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

static char prog_doc[] = N_("Remove a GNU MH folder");

int explicit_folder; /* Was the folder explicitly given */
int interactive; /* Ask for confirmation before deleting */
int recursive;     /* Recursively process all the sub-directories */

static char *cur_folder_path; /* Full pathname of the current folder */
static char *folder_name;     /* Name of the (topmost) folder to be
				 deleted */

static void
set_folder (struct mu_parseopt *po, struct mu_option *opt, char const *arg)
{
  explicit_folder = 1;
  folder_name = mu_strdup (arg);
} 

static struct mu_option options[] = {
  { "folder",  0, N_("FOLDER"), MU_OPTION_DEFAULT,
    N_("specify the folder to delete"),
    mu_c_string, NULL, set_folder },
  { "interactive", 0, NULL, MU_OPTION_DEFAULT,
    N_("interactive mode: ask for confirmation before removing each folder"),
    mu_c_bool, &interactive },
  { "recursive", 0, NULL, MU_OPTION_DEFAULT,
    N_("recursively delete all subfolders"),
    mu_c_bool, &recursive },
  MU_OPTION_END
};

static char *
current_folder_path (void)
{
  mu_mailbox_t mbox = mh_open_folder (mh_current_folder (), MU_STREAM_RDWR);
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
  mu_mailbox_t mbox = NULL;
  int rc;
  
  rc = mu_mailbox_create_default (&mbox, name);
  if (rc)
    {
      mu_error (_("cannot create mailbox %s: %s"),
		name, strerror (rc));
      return 1;
    }

  rc = mu_mailbox_remove (mbox);
  mu_mailbox_destroy (&mbox);
  if (rc)
    {
      mu_error (_("cannot remove %s: %s"), name, mu_strerror (rc));
      return 1;
    }
  return 0;
}

/* Recursive rmf */
static int
recrmf (const char *name)
{
  DIR *dir;
  struct dirent *entry;
  int failures = 0;
  
  dir = opendir (name);

  if (!dir)
    {
      mu_error (_("cannot scan folder %s: %s"), name, strerror (errno));
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
      
      p = mh_safe_make_file_name (name, entry->d_name);
      if (stat (p, &st) < 0)
	{
	  mu_diag_funcall (MU_DIAG_ERROR, "stat", p, errno);
	}
      else if (S_ISDIR (st.st_mode))
	failures += recrmf (p);
      free (p);
    }
  closedir (dir);

  if (failures == 0)
    failures += rmf (name);
  else
    printf ("%s: folder `%s' not removed\n",
	    mu_program_name, name);

  return failures;
}

int
main (int argc, char **argv)
{
  int status;
  char *name;

  mh_getopt (&argc, &argv, options, 0, NULL, prog_doc, NULL);

  cur_folder_path = current_folder_path ();

  if (!explicit_folder)
    {
      interactive = 1;
      name = cur_folder_path;
    }
  else
    name = mh_expand_name (NULL, folder_name, NAME_FOLDER);
  if (recursive)
    status = recrmf (name);
  else
    {
      if (interactive && !mh_getyn (_("Remove folder %s"), name))
	exit (0);
      status = rmf (name);
    }
  if (status == 0)
    {
      if (cur_folder_path && strcmp (name, cur_folder_path) == 0)
	{
	  mh_set_current_folder ("inbox");
	  mh_global_save_state ();
	  printf ("[+inbox now current]\n");
	}
      return 0;
    }      
  return 1;
}
