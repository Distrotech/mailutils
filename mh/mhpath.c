/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 2002 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* MH mhpath command */

#include <mh.h>

const char *argp_program_version = "mhpath (" PACKAGE_STRING ")";
static char doc[] = "GNU MH mhpath";
static char args_doc[] = "[+folder] [msgs]";

/* GNU options */
static struct argp_option options[] = {
  {"folder",  'f', "FOLDER", 0, "Specify folder to operate upon"},
  { "\nUse -help switch to obtain the list of traditional MH options. ", 0, 0, OPTION_DOC, "" },
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
mhpath (mailbox_t mbox, message_t msg, size_t num, void *data)
{
  size_t uid;
      
  mh_message_number (msg, &uid);
  printf ("%s/%lu\n", (char*) data, (unsigned long) uid);
}

int
main (int argc, char **argv)
{
  int index = 0;
  mailbox_t mbox = NULL;
  url_t url = NULL;
  char *mhdir;
  size_t total;
  mh_msgset_t msgset;
  int status;
  
  mh_argp_parse (argc, argv, options, mh_option, args_doc, doc,
		 opt_handler, NULL, &index);

  /* If  the  only argument  is `+', your MH Path is output; this
     can be useful is shell scripts. */
  if (current_folder[0] == 0)
    {
      printf ("%s\n", mu_path_folder_dir);
      exit (0);
    }
  
  mbox = mh_open_folder (current_folder, 0);

  mailbox_messages_count (mbox, &total);
  mailbox_get_url (mbox, &url);
  mhdir = (char*) url_to_string (url);
  if (strncmp (mhdir, "mh:", 3) == 0)
    mhdir += 3;

  /* If no `msgs' are specified, mhpath outputs the folder pathname
     instead.  */
  if (index == argc)
    {
      printf ("%s\n", mhdir);
      exit (0);
    }
  
  /* the name "new" has  been  added  to  mhpath's  list  of
     reserved  message  names  (the others are "first", "last",
     "prev", "next", "cur", and "all").   The  new  message  is
     equivalent  to  the  message  after  the last message in a
     folder (and equivalent to 1 in a folder without messages).
     The  "new"  message  may  not be used as part of a message
     range. */
  
  if (argc - index == 1 && strcmp (argv[index], "new") == 0)
    {
      message_t msg = NULL;
      size_t num;
      
      mailbox_get_message (mbox, total, &msg);
      mh_message_number (msg, &num);
      printf ("%s/%lu\n", mhdir, (unsigned long)(num + 1));
      exit (0);
    }
      
  /* Mhpath  expands  and  sorts  the  message  list `msgs' and
     writes the full pathnames of the messages to the  standard
     output separated by newlines. */
  mh_msgset_parse (mbox, &msgset, argc - index, argv + index, "cur");
  status = mh_iterate (mbox, &msgset, mhpath, mhdir);
  mailbox_close (mbox);
  mailbox_destroy (&mbox);
  return status;
}
