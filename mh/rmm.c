/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2002 Free Software Foundation, Inc.

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

/* MH rmm command */

#include <mh.h>

const char *argp_program_version = "rmm (" PACKAGE_STRING ")";
static char doc[] = N_("GNU MH rmm\v"
"Use -help to obtain the list of traditional MH options.");
static char args_doc[] = N_("[+folder] [msgs]");

/* GNU options */
static struct argp_option options[] = {
  {"folder",  'f', N_("FOLDER"), 0, N_("Specify folder to operate upon")},
  { 0 }
};

/* Traditional MH options */
struct mh_option mh_option[] = {
  { 0 }
};

static int
opt_handler (int key, char *arg, void *unused)
{
  switch (key)
    {
    case '+':
    case 'f': 
      current_folder = arg;
      break;
      
    default:
      return 1;
    }
  return 0;
}

void
rmm (mailbox_t mbox, message_t msg, size_t num, void *data)
{
  attribute_t attr;
  message_get_attribute (msg, &attr);
  attribute_set_deleted (attr);
}

int
main (int argc, char **argv)
{
  int index = 0;
  mailbox_t mbox;
  mh_msgset_t msgset;
  int status;

  /* Native Language Support */
  mu_init_nls ();

  mh_argp_parse (argc, argv, options, mh_option, args_doc, doc,
		 opt_handler, NULL, &index);

  mbox = mh_open_folder (current_folder, 0);

  mh_msgset_parse (mbox, &msgset, argc - index, argv + index, "cur");

  status = mh_iterate (mbox, &msgset, rmm, NULL);

  mailbox_expunge (mbox);
  mailbox_close (mbox);
  mailbox_destroy (&mbox);
  return status;
}

