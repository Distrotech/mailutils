/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003 Free Software Foundation, Inc.

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

/* MH comp command */

#include <mh.h>
#include <sys/types.h>
#include <sys/stat.h>

const char *argp_program_version = "comp (" PACKAGE_STRING ")";
static char doc[] = N_("GNU MH comp\v"
"Use -help to obtain the list of traditional MH options.");
static char args_doc[] = "[msg]";

#define ARG_NOEDIT      1
#define ARG_WHATNOWPROC 2
#define ARG_NOWHATNOWPROC 3
#define ARG_NODRAFTFOLDER 4
#define ARG_FILE 5

/* GNU options */
static struct argp_option options[] = {
  {"build", 'b', 0, 0,
   N_("Build the draft and quit immediately.")},
  {"draftfolder", 'd', N_("FOLDER"), 0,
   N_("Specify the folder for message drafts")},
  {"nodraftfolder", ARG_NODRAFTFOLDER, 0, 0,
   N_("Undo the effect of the last --draftfolder option")},
  {"draftmessage" , 'm', N_("MSG"), 0,
   N_("Invoke the draftmessage facility")},
  {"folder",  'f', N_("FOLDER"), 0, N_("Specify folder to operate upon")},
  {"file",    ARG_FILE, N_("FILE"), 0, N_("Use FILE as the message draft")},
  {"editor",  'e', N_("PROG"), 0, N_("Set the editor program to use")},
  {"noedit", ARG_NOEDIT, 0, 0, N_("Suppress the initial edit")},
  {"form",   'F', N_("FILE"), 0, N_("Read format from given file")},
  {"whatnowproc", ARG_WHATNOWPROC, N_("PROG"), 0,
   N_("Set the replacement for whatnow program")},
  {"use", 'u', N_("BOOL"), OPTION_ARG_OPTIONAL, N_("Use draft file preserved after the last session") },
  { 0 }
};

/* Traditional MH options */
struct mh_option mh_option[] = {
  {"build", 1, NULL, },
  {"file", 2,  NULL, MH_OPT_ARG, "draftfile"},
  {"form", 2,  NULL, MH_OPT_ARG, "formatfile"},
  {"draftfolder", 6, NULL, MH_OPT_ARG, "folder"},
  {"nodraftfolder", 3, NULL },
  {"draftmessage", 6, NULL },
  {"editor", 1, NULL, MH_OPT_ARG, "program"},
  {"noedit", 3, NULL, },
  {"whatnowproc", 2, NULL, MH_OPT_ARG, "program"},
  {"nowhatnowproc", 3, NULL },
  { 0 }
};

static char *format_str =
"To:\n"
"cc:\n"
"Subject:\n"
"--------\n";

struct mh_whatnow_env wh_env = { 0 };
const char *formfile;
static int initial_edit = 1;
static int build_only = 0; /* --build flag */
static int query_mode = 0; /* --query flag */
static int use_draft = 0;  /* --use flag */

static int
opt_handler (int key, char *arg, void *unused)
{
  switch (key)
    {
    case 'b':
    case ARG_NOWHATNOWPROC:
      build_only = 1;
      break;
      
    case 'd':
      wh_env.draftfolder = arg;
      break;
      
    case 'e':
      wh_env.editor = arg;
      break;
      
    case '+':
    case 'f': 
      current_folder = arg;
      break;

    case 'F':
      formfile = mh_expand_name (MHLIBDIR, arg, 0);
      break;

    case 'm':
      wh_env.draftmessage = arg;
      break;

    case 'u':
      use_draft = is_true (arg);
      break;

    case ARG_FILE:
      wh_env.draftfile = mh_expand_name (NULL, arg, 0);
      break;
	
    case ARG_NODRAFTFOLDER:
      wh_env.draftfolder = NULL;
      break;

    case ARG_NOEDIT:
      initial_edit = 0;
      break;
      
    case ARG_WHATNOWPROC:
      mh_error (_("option is not yet implemented"));
      exit (1);
      
    default:
      return 1;
    }
  return 0;
}

int 
check_draft_disposition (struct mh_whatnow_env *wh)
{
  struct stat st;
  int disp = DISP_REPLACE;

  /* First check if the draft exists */
  if (stat (wh->draftfile, &st) == 0)
    {
      if (use_draft)
	disp = DISP_USE;
      else
	{
	  printf (_("Draft \"%s\" exists (%lu bytes).\n"),
		  wh->draftfile, (unsigned long) st.st_size);
	  disp = mh_disposition (wh->draftfile);
	}
    }

  return disp;
}
  
int
copy_message (mailbox_t mbox, size_t n, const char *file)
{
  message_t msg;
  stream_t in;
  stream_t out;
  int rc;
  size_t size;
  char *buffer;
  size_t bufsize, rdsize;
  
  mailbox_get_message (mbox, n, &msg);
  message_size (msg, &size);

  for (bufsize = size; bufsize > 0 && (buffer = malloc (bufsize)) == 0;
       bufsize /= 2)
    ;

  if (!bufsize)
    mh_err_memory (1);

  message_get_stream (msg, &in);
  
  if ((rc = file_stream_create (&out,
				file, MU_STREAM_RDWR|MU_STREAM_CREAT)) != 0
      || (rc = stream_open (out)))
    {
      mh_error (_("cannot open output file \"%s\": %s"),
		file, mu_strerror (rc));
      free (buffer);
      return 1;
    }

  while (size > 0
	 && (rc = stream_sequential_read (in, buffer, bufsize, &rdsize)) == 0
	 && rdsize > 0)
    {
      if ((rc = stream_sequential_write (out, buffer, rdsize)) != 0)
	{
	  mh_error (_("error writing to \"%s\": %s"),
		    file, mu_strerror (rc));
	  break;
	}
      size -= rdsize;
    }

  stream_close (out);
  stream_destroy (&out, stream_get_owner (out));
  
  return rc;
}

int
main (int argc, char **argv)
{
  int index;
  
  /* Native Language Support */
  mu_init_nls ();

  mh_argp_parse (argc, argv, options, mh_option, args_doc, doc,
		 opt_handler, NULL, &index);

  if (!wh_env.draftfolder)
    wh_env.draftfolder = mh_global_profile_get ("Draft-Folder",
						mu_path_folder_dir);

  wh_env.file = mh_expand_name (wh_env.draftfolder, "comp", 0);
  if (!wh_env.draftfile)
    wh_env.draftfile = mh_expand_name (wh_env.draftfolder, "draft", 0);

  switch (check_draft_disposition (&wh_env))
    {
    case DISP_QUIT:
      exit (0);

    case DISP_USE:
      unlink (wh_env.file);
      rename (wh_env.draftfile, wh_env.file);
      break;
	  
    case DISP_REPLACE:
      unlink (wh_env.draftfile);
  
      if (index < argc)
	{
	  static mh_msgset_t msgset;
	  static mailbox_t mbox;
	  
	  mbox = mh_open_folder (current_folder, 0);
	  mh_msgset_parse (mbox, &msgset, argc - index, argv + index, "cur");
	  if (msgset.count != 1)
	    {
	      mh_error (_("only one message at a time!"));
	      return 1;
	    }
	  copy_message (mbox, msgset.list[0], wh_env.file);
	}
      else if (formfile)
	{
	  if (mh_file_copy (formfile, wh_env.file) == 0)
	    exit (1);
	}
      else
	{
	  int rc;
	  stream_t stream;
	  
	  if ((rc = file_stream_create (&stream,
					wh_env.file,
					MU_STREAM_WRITE|MU_STREAM_CREAT)) != 0
	      || (rc = stream_open (stream)))
	    {
	      mh_error (_("cannot open output file \"%s\": %s"),
			wh_env.file, mu_strerror (rc));
	      exit (1);
	    }
	  
	  rc = stream_sequential_write (stream, 
					format_str, strlen (format_str));
	  stream_close (stream);
	  stream_destroy (&stream, stream_get_owner (stream));

	  if (rc)
	    {
	      mh_error (_("error writing to \"%s\": %s"),
			wh_env.file, mu_strerror (rc));
	      exit (1);
	    }
	}
    }
  
  /* Exit immediately if --build is given */
  if (build_only)
    return 0;

  return mh_whatnow (&wh_env, initial_edit);
}
