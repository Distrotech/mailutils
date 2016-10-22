/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003, 2008-2012, 2014-2016 Free Software Foundation,
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

/* MH whatnow command */

#include <mh.h>

static char prog_doc[] = "GNU MH whatnow";
static char args_doc[] = N_("[FILE]");

struct mh_whatnow_env wh_env = { 0 };
static int initial_edit = 1;
static char *draftmessage = "cur";
static const char *draftfolder = NULL;

static struct mu_option options[] = {
  { "draftfolder",   0,      N_("FOLDER"), MU_OPTION_DEFAULT,
    N_("specify the folder for message drafts"),
    mu_c_string, &draftfolder },
  { "nodraftfolder", 0, NULL, MU_OPTION_DEFAULT,
    N_("undo the effect of the last -draftfolder option"),
    mu_c_string, &draftfolder, mh_opt_clear_string },
  { "draftmessage" , 0, N_("MSG"), MU_OPTION_DEFAULT,
    N_("invoke the draftmessage facility"),
    mu_c_string, &draftmessage },
  { "editor",        0, N_("PROG"), MU_OPTION_DEFAULT,
    N_("set the editor program to use"),
    mu_c_string, &wh_env.editor },
  { "noedit",        0, NULL, MU_OPTION_DEFAULT,
    N_("suppress the initial edit"),
    mu_c_int, &initial_edit, NULL, "0" },
  { "prompt", 0, N_("STRING"), MU_OPTION_DEFAULT,
    N_("set the prompt"),
    mu_c_string, &wh_env.prompt },
  MU_OPTION_END
};

int
main (int argc, char **argv)
{
  MU_APP_INIT_NLS ();

  mh_whatnow_env_from_environ (&wh_env);

  mh_getopt (&argc, &argv, options, 0, args_doc, prog_doc, NULL);
  
  if (argc)
    wh_env.draftfile = argv[0];
  else if (draftfolder)
    {
      if (mh_draft_message (draftfolder, draftmessage, &wh_env.file))
	return 1;
    }
  else
    wh_env.draftfile = mh_expand_name (draftfolder, "draft", NAME_ANY);
  wh_env.draftfile = wh_env.file;

  return mh_whatnow (&wh_env, initial_edit);
}
  
