/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2002,2003 Free Software Foundation, Inc.

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

/* MH refile command */

#include <mh.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

const char *program_version = "refile (" PACKAGE_STRING ")";
static char doc[] = N_("GNU MH refile\v"
"Options marked with `*' are not yet implemented.\n"
"Use -help to obtain the list of traditional MH options.");
static char args_doc[] = N_("messages folder [folder...]");

/* GNU options */
static struct argp_option options[] = {
  {"folder",  ARG_FOLDER, N_("FOLDER"), 0,
   N_("Specify folder to operate upon")},
  {"draft",   ARG_DRAFT, NULL, 0,
   N_("Use <mh-dir>/draft as the source message")},
  {"copy",    ARG_LINK, N_("BOOL"), OPTION_ARG_OPTIONAL,
   N_("Preserve the source folder copy.")},
  {"link",    0, NULL, OPTION_ALIAS, NULL},
  {"preserve", ARG_PRESERVE, N_("BOOL"), OPTION_ARG_OPTIONAL,
   N_("* Try to preserve message sequence numbers")},
  {"source", ARG_SOURCE, N_("FOLDER"), 0,
   N_("Specify source folder. FOLDER will become the current folder after the program exits.")},
  {"src", 0, NULL, OPTION_ALIAS, NULL},
  {"file", ARG_FILE, N_("FILE"), 0, N_("Use FILE as the source message")},
  {"license", ARG_LICENSE, 0,      0,
   N_("Display software license"), -1},
  { 0 }
};

/* Traditional MH options */
struct mh_option mh_option[] = {
  {"file",     2, 0, "input-file"},
  {"draft",    1, 0, NULL },
  {"link",     1, MH_OPT_BOOL, NULL },
  {"preserve", 1, MH_OPT_BOOL, NULL },
  {"src",      1, 0, "+folder" },
  { 0 }
};

int link_flag = 0;
int preserve_flag = 0;
char *source_file = NULL;
list_t folder_name_list = NULL;
list_t folder_mbox_list = NULL;

void
add_folder (const char *folder)
{
  if (!folder_name_list && list_create (&folder_name_list))
    {
      mh_error (_("can't create folder list"));
      exit (1);
    }
  list_append (folder_name_list, strdup (folder));
}

void
open_folders ()
{
  iterator_t itr;

  if (!folder_name_list)
    {
      mh_error (_("no folder specified"));
      exit (1);
    }

  if (list_create (&folder_mbox_list))
    {
      mh_error (_("can't create folder list"));
      exit (1);
    }

  if (iterator_create (&itr, folder_name_list))
    {
      mh_error (_("can't create iterator"));
      exit (1);
    }

  for (iterator_first (itr); !iterator_is_done (itr); iterator_next (itr))
    {
      char *name = NULL;
      mailbox_t mbox;
      
      iterator_current (itr, (void **)&name);
      mbox = mh_open_folder (name, 1);
      list_append (folder_mbox_list, mbox);
      free (name);
    }
  iterator_destroy (&itr);
  list_destroy (&folder_name_list);
}

void
enumerate_folders (void (*f) __P((void *, mailbox_t)), void *data)
{
  iterator_t itr;

  if (iterator_create (&itr, folder_mbox_list))
    {
      mh_error (_("can't create iterator"));
      exit (1);
    }

  for (iterator_first (itr); !iterator_is_done (itr); iterator_next (itr))
    {
      mailbox_t mbox;
      iterator_current (itr, (void **)&mbox);
      (*f) (data, mbox);
    }
  iterator_destroy (&itr);
}
  
void
_close_folder (void *unused, mailbox_t mbox)
{
  mailbox_close (mbox);
  mailbox_destroy (&mbox);
}

void
close_folders ()
{
  enumerate_folders (_close_folder, NULL);
}

static int
opt_handler (int key, char *arg, void *unused, struct argp_state *state)
{
  switch (key)
    {
    case ARG_FOLDER: 
      add_folder (arg);
      break;

    case ARG_DRAFT:
      source_file = "draft";
      break;

    case ARG_LINK:
      link_flag = is_true(arg);
      break;
      
    case ARG_PRESERVE:
      preserve_flag = is_true(arg);
      break;
	
    case ARG_SOURCE:
      current_folder = arg;
      break;
      
    case ARG_FILE:
      source_file = arg;
      break;
      
    case ARG_LICENSE:
      mh_license (argp_program_version);
      break;

    default:
      return 1;
    }
  return 0;
}

void
refile_folder (void *data, mailbox_t mbox)
{
  message_t msg = data;
  int rc;
  
  rc = mailbox_append_message (mbox, msg);
  if (rc)
    {
      mh_error (_("error appending message: %s"), mu_strerror (rc));
      exit (1);
    }
}

void
refile (message_t msg)
{
  enumerate_folders (refile_folder, msg);
}

void
refile_iterator (mailbox_t mbox, message_t msg, size_t num, void *data)
{
  enumerate_folders (refile_folder, msg);
  if (!link_flag)
    {
      attribute_t attr;
      message_get_attribute (msg, &attr);
      attribute_set_deleted (attr);
    }
}

int
main (int argc, char **argv)
{
  int index;
  mh_msgset_t msgset;
  mailbox_t mbox;
  int status, i, j;

  /* Native Language Support */
  mu_init_nls ();

  mu_argp_init (program_version, NULL);
  mh_argp_parse (&argc, &argv, 0, options, mh_option, args_doc, doc,
		 opt_handler, NULL, &index);

  argc -= index;
  argv += index;

  /* Collect any surplus folders */
  for (i = j = 0; i < argc; i++)
    {
      if (argv[i][0] == '+')
	add_folder (argv[i]);
      else
	argv[j++] = argv[i];
    }
  argv[j] = NULL;
  argc = j;
  
  open_folders ();

  if (source_file)
    {
      message_t msg;
      
      if (argc > 0)
	{
	  mh_error (_("both message set and source file given"));
	  exit (1);
	}
      msg = mh_file_to_message (mu_path_folder_dir, source_file);
      refile (msg);
      if (!link_flag)
	unlink (source_file);
      status = 0;
    }
  else
    {
      mbox = mh_open_folder (current_folder, 0);
      mh_msgset_parse (mbox, &msgset, argc, argv, "cur");

      status = mh_iterate (mbox, &msgset, refile_iterator, NULL);
 
      mailbox_expunge (mbox);
      mailbox_close (mbox);
      mailbox_destroy (&mbox);
    }

  close_folders ();
  
  return status;
}
