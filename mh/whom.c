/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003, 2007-2012, 2014-2016 Free Software Foundation,
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

#include <mh.h>

static char prog_doc[] = N_("GNU MH whom");
static char args_doc[] = "[FILE]";

static int check_recipients;
static int use_draft;            /* Use the prepared draft */
static const char *draft_folder; /* Use this draft folder */

static void
add_alias (struct mu_parseopt *po, struct mu_option *opt, char const *arg)
{
  mh_alias_read (arg, 1);
}

static void
set_draftmessage (struct mu_parseopt *po, struct mu_option *opt,
		  char const *arg)
{
  if (!draft_folder)
    draft_folder = mh_global_profile_get ("Draft-Folder",
					  mu_folder_directory ());
}

static struct mu_option options[] = {
  { "alias",        0,    N_("FILE"), MU_OPTION_DEFAULT,
    N_("specify additional alias file"),
    mu_c_string, NULL, add_alias },
  { "draft",        0,    NULL, MU_OPTION_DEFAULT,
    N_("use prepared draft"),
    mu_c_bool, &use_draft },
  { "draftfolder",   0,      N_("FOLDER"), MU_OPTION_DEFAULT,
    N_("specify the folder for message drafts"),
    mu_c_string, &draft_folder },
  { "nodraftfolder", 0, NULL, MU_OPTION_DEFAULT,
    N_("undo the effect of the last -draftfolder option"),
    mu_c_string, &draft_folder, mh_opt_clear_string },
  { "draftmessage",  0,  NULL, MU_OPTION_DEFAULT,
    N_("treat the arguments as a list of messages from the draftfolder"),
    mu_c_string, NULL, set_draftmessage },
  { "check",         0,  NULL, MU_OPTION_DEFAULT,
    N_("check if addresses are deliverable"),
    mu_c_bool, &check_recipients },
  MU_OPTION_END
};

int
main (int argc, char **argv)
{
  char *name = "draft";
  
  MU_APP_INIT_NLS ();
  
  mh_getopt (&argc, &argv, options, 0, args_doc, prog_doc, NULL);

  if (!use_draft && argc > 0)
    name = argv[0];

  if (!draft_folder)
    draft_folder = mh_global_profile_get ("Draft-Folder",
					  mu_folder_directory ());

  
  return mh_whom (mh_expand_name (draft_folder, name, NAME_ANY), 
                  check_recipients) ? 1 : 0;
}
