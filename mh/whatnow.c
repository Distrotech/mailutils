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

/* MH whatnow command */

#include <mh.h>

const char *argp_program_version = "whatnow (" PACKAGE_STRING ")";
static char doc[] = "GNU MH whatnow";
static char args_doc[] = N_("[FILE]");

#define ARG_NODRAFTFOLDER 1
#define ARG_NOEDIT        2

/* GNU options */
static struct argp_option options[] = {
  {"draftfolder", 'd', N_("FOLDER"), 0,
   N_("Specify the folder for message drafts")},
  {"nodraftfolder", ARG_NODRAFTFOLDER, 0, 0,
   N_("Undo the effect of the last --draftfolder option")},
  {"draftmessage" , 'm', N_("MSG"), 0,
   N_("Invoke the draftmessage facility")},
  {"editor",  'e', N_("PROG"), 0, N_("Set the editor program to use")},
  {"noedit", ARG_NOEDIT, 0, 0, N_("Suppress the initial edit")},
  {"prompt", 'p', N_("STRING"), 0, N_("Set the prompt")},
  { NULL }
};

/* Traditional MH options */
struct mh_option mh_option[] = {
  {"draftfolder", 6, MH_OPT_ARG, "folder"},
  {"nodraftfolder", 3, },
  {"draftmessage", 6, },
  {"editor", 1, MH_OPT_ARG, "program"},
  {"noedit", 3, },
  {"prompt", 1 },
  { 0 }
};

struct mh_whatnow_env wh_env = { 0 };
static int initial_edit = 1;

static int
opt_handler (int key, char *arg, void *unused)
{
  switch (key)
    {
    case 'd':
      wh_env.draftfolder = arg;
      break;
      
    case 'e':
      wh_env.editor = arg;
      break;
      
    case ARG_NODRAFTFOLDER:
      wh_env.draftfolder = NULL;
      break;

    case ARG_NOEDIT:
      initial_edit = 0;
      break;

    case 'm':
      wh_env.draftmessage = arg;
      break;

    case 'p':
      wh_env.prompt = arg;
      break;
      
    default:
      return 1;
    }
  return 0;
}

int
main (int argc, char **argv)
{
  int index;
  
  mu_init_nls ();

  mh_argp_parse (argc, argv, options, mh_option, args_doc, doc,
		 opt_handler, NULL, &index);

  wh_env.msg = getenv ("mhaltmsg");
  return mh_whatnow (&wh_env, initial_edit);
}
  
