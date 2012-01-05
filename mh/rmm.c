/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2002, 2005, 2007-2012 Free Software Foundation, Inc.

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

/* MH rmm command */

#include <mh.h>

static char doc[] = N_("GNU MH rmm")"\v"
N_("Use -help to obtain the list of traditional MH options.");
static char args_doc[] = N_("[+FOLDER] [MSGLIST]");

/* GNU options */
static struct argp_option options[] = {
  {"folder",  ARG_FOLDER, N_("FOLDER"), 0,
   N_("specify folder to operate upon")},
  { 0 }
};

/* Traditional MH options */
struct mh_option mh_option[] = {
  { NULL }
};

static error_t
opt_handler (int key, char *arg, struct argp_state *state)
{
  switch (key)
    {
    case ARG_FOLDER: 
      mh_set_current_folder (arg);
      break;
      
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

static int
rmm (size_t num, mu_message_t msg, void *data)
{
  mu_attribute_t attr;
  mu_message_get_attribute (msg, &attr);
  mu_attribute_set_deleted (attr);
  return 0;
}

int
main (int argc, char **argv)
{
  int index = 0;
  mu_mailbox_t mbox;
  mu_msgset_t msgset;
  int status;

  /* Native Language Support */
  MU_APP_INIT_NLS ();

  mh_argp_init ();
  mh_argp_parse (&argc, &argv, 0, options, mh_option, args_doc, doc,
		 opt_handler, NULL, &index);

  mbox = mh_open_folder (mh_current_folder (), MU_STREAM_RDWR);

  mh_msgset_parse (&msgset, mbox, argc - index, argv + index, "cur");

  status = mu_msgset_foreach_message (msgset, rmm, NULL);

  mu_mailbox_expunge (mbox);
  mu_mailbox_close (mbox);
  mu_mailbox_destroy (&mbox);
  mh_global_save_state ();
  return status;
}

