/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003, 2008-2012 Free Software Foundation, Inc.

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

/* MH whatnow command */

#include <mh.h>

static char doc[] = "GNU MH whatnow";
static char args_doc[] = N_("[FILE]");

/* GNU options */
static struct argp_option options[] = {
  {"draftfolder", ARG_DRAFTFOLDER, N_("FOLDER"), 0,
   N_("specify the folder for message drafts")},
  {"nodraftfolder", ARG_NODRAFTFOLDER, 0, 0,
   N_("undo the effect of the last --draftfolder option")},
  {"draftmessage" , ARG_DRAFTMESSAGE, N_("MSG"), 0,
   N_("invoke the draftmessage facility")},
  {"editor",  ARG_EDITOR, N_("PROG"), 0, N_("set the editor program to use")},
  {"noedit", ARG_NOEDIT, 0, 0, N_("suppress the initial edit")},
  {"prompt", ARG_PROMPT, N_("STRING"), 0, N_("set the prompt")},

  { NULL }
};

/* Traditional MH options */
struct mh_option mh_option[] = {
  { "draftfolder",  MH_OPT_ARG, "folder" },
  { "nodraftfolder" },
  { "draftmessage" },
  { "editor",       MH_OPT_ARG, "program" },
  { "noedit" },
  { "prompt" },
  { NULL }
};

struct mh_whatnow_env wh_env = { 0 };
static int initial_edit = 1;
static char *draftmessage = "cur";
static const char *draftfolder = NULL;

static error_t
opt_handler (int key, char *arg, struct argp_state *state)
{
  switch (key)
    {
    case ARG_DRAFTFOLDER:
      draftfolder = arg;
      break;
      
    case ARG_EDITOR:
      wh_env.editor = arg;
      break;
      
    case ARG_NODRAFTFOLDER:
      draftfolder = NULL;
      break;

    case ARG_NOEDIT:
      initial_edit = 0;
      break;

    case ARG_DRAFTMESSAGE:
      draftmessage = arg;
      break;

    case ARG_PROMPT:
      wh_env.prompt = arg;
      break;
      
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

int
main (int argc, char **argv)
{
  int index;
  
  MU_APP_INIT_NLS ();

  mh_argp_init ();
  mh_whatnow_env_from_environ (&wh_env);
  mh_argp_parse (&argc, &argv, 0, options, mh_option, args_doc, doc,
		 opt_handler, NULL, &index);
  argc -= index;
  argv += index;
  if (argc)
    wh_env.draftfile = argv[0];
  else if (draftfolder)
    {
      if (mh_draft_message (draftfolder, draftmessage, &wh_env.file))
	return 1;
    }
  else
    wh_env.draftfile = mh_expand_name (draftfolder, "draft", 0);
  wh_env.draftfile = wh_env.file;

  return mh_whatnow (&wh_env, initial_edit);
}
  
