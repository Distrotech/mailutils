/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2010 Free Software Foundation, Inc.

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

/* MH mhparam command */

#include <mh.h>

static char doc[] = N_("GNU MH mhseq")"\v"
N_("Use -help to obtain the list of traditional MH options.");
static char args_doc[] = N_("[SEQUENCE]");

static struct argp_option options[] = {
  {"folder",  ARG_FOLDER, N_("FOLDER"), 0,
   N_("specify the folder to use")},
  { "uids",  'u', NULL, 0,
    N_("show message UIDs (default)")},
  { "numbers", 'n', NULL, 0,
    N_("show message numbers") },
  { NULL }
};

/* Traditional MH options */
struct mh_option mh_option[] = {
  { "uid" },
  { NULL }
};

static int uid_option = 1;

static error_t
opt_handler (int key, char *arg, struct argp_state *state)
{
  switch (key)
    {
    case ARG_FOLDER: 
      mh_set_current_folder (arg);
      break;

    case 'n':
      uid_option = 0;
      break;
      
    case 'u':
      uid_option = 1;
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
  mu_mailbox_t mbox;
  mh_msgset_t msgset;
  size_t i;
    
  /* Native Language Support */
  MU_APP_INIT_NLS ();

  mh_argp_init ();
  mh_argp_parse (&argc, &argv, 0, options, mh_option, args_doc, doc,
		 opt_handler, NULL, &index);

  argc -= index;
  argv += index;
  mbox = mh_open_folder (mh_current_folder (), MU_STREAM_RDWR);

  mh_msgset_parse (mbox, &msgset, argc, argv, "cur");
  if (uid_option)
    mh_msgset_uids (mbox, &msgset);

  for (i = 0; i < msgset.count; i++)
    printf ("%lu\n", (unsigned long) msgset.list[i]);
  return 0;
}

  
