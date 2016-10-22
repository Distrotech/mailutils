/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003, 2005-2012, 2014-2016 Free Software Foundation,
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

/* MH comp command */

#include <mh.h>
#include <sys/types.h>
#include <sys/stat.h>

static char prog_doc[] = N_("Compose a message");
static char args_doc[] = N_("[MSG]");

struct mh_whatnow_env wh_env = { 0 };
static int initial_edit = 1;
static const char *whatnowproc;
static int nowhatnowproc;
char *formfile;
static int build_only = 0; /* -build flag */
static int use_draft = 0;  /* -use flag */
static char *draftmessage = "new";
static const char *draftfolder = NULL;
static int folder_set; /* Folder is set on the command line */

static void
set_folder (struct mu_parseopt *po, struct mu_option *opt, char const *arg)
{
  mh_set_current_folder (arg);
  folder_set = 1;
}

static void
set_file (struct mu_parseopt *po, struct mu_option *opt, char const *arg)
{
  wh_env.file = mh_expand_name (NULL, arg, NAME_ANY);
}

static struct mu_option options[] = {
  { "build",        0, NULL, MU_OPTION_DEFAULT,
    N_("build the draft and quit immediately."),
    mu_c_bool, &build_only },
  { "draftfolder",  0, N_("FOLDER"), MU_OPTION_DEFAULT,
    N_("specify the folder for message drafts"),
    mu_c_string, &draftfolder },
  { "nodraftfolder", 0, NULL, MU_OPTION_DEFAULT,
    N_("undo the effect of the last --draftfolder option"),
    mu_c_string, &draftfolder, mh_opt_clear_string },
  { "draftmessage" , 0, N_("MSG"), MU_OPTION_DEFAULT,
    N_("invoke the draftmessage facility"),
    mu_c_string, &draftmessage },
  { "folder",        0, N_("FOLDER"), MU_OPTION_DEFAULT,
    N_("specify folder to operate upon"),
    mu_c_string, NULL, set_folder },
  { "file",          0, N_("FILE"), MU_OPTION_DEFAULT,
    N_("use FILE as the message draft"),
    mu_c_string, NULL, set_file },
  { "editor",        0, N_("PROG"), MU_OPTION_DEFAULT,
    N_("set the editor program to use"),
    mu_c_string, &wh_env.editor },
  { "noedit",        0, NULL, MU_OPTION_DEFAULT,
    N_("suppress the initial edit"),
    mu_c_int, &initial_edit, NULL, "0" },
  { "form",          0, N_("FILE"), MU_OPTION_DEFAULT,
    N_("read format from given file"),
    mu_c_string, &formfile, mh_opt_find_file },
  { "whatnowproc",   0, N_("PROG"), MU_OPTION_DEFAULT,
    N_("set the replacement for whatnow program"),
    mu_c_string, &whatnowproc },
  { "nowhatnowproc", 0, NULL, MU_OPTION_DEFAULT,
    N_("don't run whatnowproc"),
    mu_c_string, &nowhatnowproc, NULL, "1" },
  { "use",           0, NULL, MU_OPTION_DEFAULT,
    N_("use draft file preserved after the last session"),
    mu_c_bool, &use_draft },
  MU_OPTION_END
};

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
  draftfolder = mh_global_profile_get ("Draft-Folder", NULL);
  whatnowproc = mh_global_profile_get ("whatnowproc", NULL);

  mh_getopt (&argc, &argv, options, 0, args_doc, prog_doc, NULL);
  if (use_draft)
    draftmessage = "cur";
  if (!formfile)
    mh_find_file ("components", &formfile);
  
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
      wh_env.file = mh_expand_name (NULL, "draft", NAME_ANY);
    }
  else
    {
      if (build_only || !draftfolder)
	{
	  switch (argc)
	    {
	    case 0:
	      wh_env.file = mh_expand_name (NULL, "draft", 0);
	      break;

	    case 1:
	      wh_env.file = mh_expand_name (NULL, argv[0], 0);
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
	  if (use_draft || argc)
	    {
	      mu_msgset_t msgset;
	      mu_mailbox_t mbox;
	      
	      mbox = mh_open_folder (draftfolder, 
                                     MU_STREAM_RDWR|MU_STREAM_CREAT);
	      mh_msgset_parse (&msgset, mbox, 
	                       argc, argv,
			       use_draft ? "cur" : "new");
	      if (!mh_msgset_single_message (msgset))
		{
		  mu_error (_("only one message at a time!"));
		  return 1;
		}
	      draftmessage = (char*) mu_umaxtostr (0, mh_msgset_first_uid (msgset));
	      mu_msgset_free (msgset);
	      mu_mailbox_destroy (&mbox);
	    }
	  if (mh_draft_message (draftfolder, draftmessage,
				&wh_env.file))
	    return 1;
	}
    }
  wh_env.draftfile = wh_env.file;

  if (folder_set && argc)
    {
      mu_msgset_t msgset;
      mu_mailbox_t mbox;
      
      mbox = mh_open_folder (mh_current_folder (), MU_STREAM_READ);
      mh_msgset_parse (&msgset, mbox, argc, argv, "cur");
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
  
  /* Exit immediately if -build is given */
  if (build_only || nowhatnowproc)
    return 0;

  return mh_whatnowproc (&wh_env, initial_edit, whatnowproc);
}
