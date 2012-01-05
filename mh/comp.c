/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003, 2005-2012 Free Software Foundation, Inc.

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

/* MH comp command */

#include <mh.h>
#include <sys/types.h>
#include <sys/stat.h>

static char doc[] = N_("GNU MH comp")"\v"
N_("Options marked with `*' are not yet implemented.\n"
"Use -help to obtain the list of traditional MH options.");
static char args_doc[] = N_("[MSG]");

/* GNU options */
static struct argp_option options[] = {
  {"build",         ARG_BUILD, 0, 0,
   N_("build the draft and quit immediately.")},
  {"draftfolder",   ARG_DRAFTFOLDER, N_("FOLDER"), 0,
   N_("specify the folder for message drafts")},
  {"nodraftfolder", ARG_NODRAFTFOLDER, 0, 0,
   N_("undo the effect of the last --draftfolder option")},
  {"draftmessage" , ARG_DRAFTMESSAGE, N_("MSG"), 0,
   N_("invoke the draftmessage facility")},
  {"folder",        ARG_FOLDER, N_("FOLDER"), 0,
   N_("specify folder to operate upon")},
  {"file",          ARG_FILE, N_("FILE"), 0,
   N_("use FILE as the message draft")},
  {"editor",        ARG_EDITOR, N_("PROG"), 0,
   N_("set the editor program to use")},
  {"noedit",        ARG_NOEDIT, 0, 0,
   N_("suppress the initial edit")},
  {"form",          ARG_FORM, N_("FILE"), 0,
   N_("read format from given file")},
  {"whatnowproc",   ARG_WHATNOWPROC, N_("PROG"), 0,
   N_("set the replacement for whatnow program")},
  {"nowhatnowproc", ARG_NOWHATNOWPROC, NULL, 0,
   N_("ignore whatnowproc variable. Use standard `whatnow' shell instead.")},
  {"use",           ARG_USE, N_("BOOL"), OPTION_ARG_OPTIONAL,
   N_("use draft file preserved after the last session") },
  {"nouse",         ARG_NOUSE, NULL, OPTION_HIDDEN, ""},
  { 0 }
};

/* Traditional MH options */
struct mh_option mh_option[] = {
  { "build" },
  { "file",          MH_OPT_ARG, "draftfile" },
  { "form",          MH_OPT_ARG, "formatfile" },
  { "draftfolder",   MH_OPT_ARG, "folder" },
  { "nodraftfolder" },
  { "draftmessage" },
  { "editor",        MH_OPT_ARG, "program" },
  { "noedit" },
  { "whatnowproc",   MH_OPT_ARG, "program" },
  { "nowhatnowproc" },
  { "use" },
  { NULL }
};

struct mh_whatnow_env wh_env = { 0 };
static int initial_edit = 1;
static const char *whatnowproc;
char *formfile;
static int build_only = 0; /* --build flag */
static int use_draft = 0;  /* --use flag */
static char *draftmessage = "new";
static const char *draftfolder = NULL;
static int folder_set; /* Folder is set on the command line */

static error_t
opt_handler (int key, char *arg, struct argp_state *state)
{
  switch (key)
    {
    case ARGP_KEY_INIT:
      draftfolder = mh_global_profile_get ("Draft-Folder", NULL);
      whatnowproc = mh_global_profile_get ("whatnowproc", NULL);
      break;

    case ARG_BUILD:
      build_only = 1;
      break;
      
    case ARG_DRAFTFOLDER:
      draftfolder = arg;
      break;
      
    case ARG_EDITOR:
      wh_env.editor = arg;
      break;
      
    case ARG_FOLDER: 
      mh_set_current_folder (arg);
      folder_set = 1;
      break;

    case ARG_FORM:
      mh_find_file (arg, &formfile);
      break;

    case ARG_DRAFTMESSAGE:
      draftmessage = arg;
      break;

    case ARG_USE:
      use_draft = is_true (arg);
      draftmessage = "cur";
      break;

    case ARG_NOUSE:
      use_draft = 0;
      break;
      
    case ARG_FILE:
      wh_env.file = mh_expand_name (NULL, arg, 0);
      break;
	
    case ARG_NODRAFTFOLDER:
      draftfolder = NULL;
      break;

    case ARG_NOEDIT:
      initial_edit = 0;
      break;
      
    case ARG_WHATNOWPROC:
      whatnowproc = arg;
      break;

    case ARG_NOWHATNOWPROC:
      whatnowproc = NULL;
      break;

    case ARGP_KEY_FINI:
      if (!formfile)
	mh_find_file ("components", &formfile);
      break;
	  
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

/* Copy Nth message from mailbox MBOX to FILE. */
int
copy_message (mu_mailbox_t mbox, size_t n, const char *file)
{
  mu_message_t msg;
  mu_stream_t in;
  mu_stream_t out;
  int rc;
  
  rc = mu_mailbox_get_message (mbox, n, &msg);
  if (rc)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_mailbox_get_message", NULL, rc);
      exit (1);
    }
  
  mu_message_get_streamref (msg, &in);
  
  if ((rc = mu_file_stream_create (&out,
				   file, MU_STREAM_RDWR|MU_STREAM_CREAT)))
    {
      mu_error (_("cannot open output file \"%s\": %s"),
		file, mu_strerror (rc));
      mu_stream_destroy (&in);
      return rc;
    }

  rc = mu_stream_copy (out, in, 0, NULL);
  mu_stream_destroy (&in);
  mu_stream_close (out);
  mu_stream_destroy (&out);
  
  if (rc)
    {
      mu_error (_("error copying to \"%s\": %s"),
		file, mu_strerror (rc));
    }
  return rc;
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
  
  if (wh_env.file)
    {
      if (build_only)
	{
	  mu_error (_("--build and --file cannot be used together"));
	  exit (1);
	}
    }
  else if (folder_set)
    {
      wh_env.file = mh_expand_name (NULL, "draft", 0);
    }
  else
    {
      if (build_only || !draftfolder)
	{
	  switch (argc - index)
	    {
	    case 0:
	      wh_env.file = mh_expand_name (NULL, "draft", 0);
	      break;

	    case 1:
	      wh_env.file = mh_expand_name (NULL, argv[index], 0);
	      break;
	      
	    default:
	      mu_error (_("only one message at a time!"));
	      return 1;
	    }
	}
      else if (draftfolder)
	{
	  /* Comp accepts a `file', and it will, if given
	     `-draftfolder +folder'  treat this arguments  as `msg'. */
	  if (use_draft || index < argc)
	    {
	      mu_msgset_t msgset;
	      mu_mailbox_t mbox;
	      
	      mbox = mh_open_folder (draftfolder, 
                                     MU_STREAM_RDWR|MU_STREAM_CREAT);
	      mh_msgset_parse (&msgset, mbox, 
	                       argc - index, argv + index,
			       use_draft ? "cur" : "new");
	      if (!mh_msgset_single_message (msgset))
		{
		  mu_error (_("only one message at a time!"));
		  return 1;
		}
	      draftmessage = mu_umaxtostr (0, mh_msgset_first_uid (msgset));
	      mu_msgset_free (msgset);
	      mu_mailbox_destroy (&mbox);
	    }
	  if (mh_draft_message (draftfolder, draftmessage,
				&wh_env.file))
	    return 1;
	}
    }
  wh_env.draftfile = wh_env.file;

  if (folder_set && index < argc)
    {
      mu_msgset_t msgset;
      mu_mailbox_t mbox;
      
      mbox = mh_open_folder (mh_current_folder (), MU_STREAM_READ);
      mh_msgset_parse (&msgset, mbox, argc - index, argv + index, "cur");
      if (!mh_msgset_single_message (msgset))
	{
	  mu_error (_("only one message at a time!"));
	  return 1;
	}
      unlink (wh_env.file);
      copy_message (mbox, mh_msgset_first (msgset), wh_env.file);
      mu_mailbox_destroy (&mbox);
      mu_msgset_free (msgset);
    }
  else
    {
      switch (check_draft_disposition (&wh_env, use_draft))
	{
	case DISP_QUIT:
	  exit (0);

	case DISP_USE:
	  break;
	  
	case DISP_REPLACE:
	  unlink (wh_env.draftfile);
	  mh_comp_draft (formfile, wh_env.file);
	}
    }
  
  /* Exit immediately if --build is given */
  if (build_only)
    return 0;

  return mh_whatnowproc (&wh_env, initial_edit, whatnowproc);
}
