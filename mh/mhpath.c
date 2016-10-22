/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2002, 2005, 2007-2012, 2014-2016 Free Software
   Foundation, Inc.

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

/* MH mhpath command */

#include <mh.h>

static char prog_doc[] = N_("Print full pathnames of GNU MH messages and folders");
static char args_doc[] = N_("[+FOLDER] [MSGLIST]");

static int
mhpath (size_t num, mu_message_t msg, void *data)
{
  size_t uid;
  
  mh_message_number (msg, &uid);
  printf ("%s/%s\n", (char*) data, mu_umaxtostr (0, uid));
  return 0;
}

int
main (int argc, char **argv)
{
  mu_mailbox_t mbox = NULL;
  mu_url_t url = NULL;
  char *mhdir;
  size_t total;
  mu_msgset_t msgset;
  int status;
  const char *current_folder;
  
  mh_getopt (&argc, &argv, NULL, MH_GETOPT_DEFAULT_FOLDER,
	     args_doc, prog_doc, NULL);

  current_folder = mh_current_folder ();
  /* If  the  only argument  is `+', your MH Path is output; this
     can be useful is shell scripts. */
  if (current_folder[0] == 0)
    {
      printf ("%s\n", mu_folder_directory ());
      exit (0);
    }
  
  mbox = mh_open_folder (current_folder, MU_STREAM_RDWR);

  mu_mailbox_messages_count (mbox, &total);
  mu_mailbox_get_url (mbox, &url);
  mhdir = (char*) mu_url_to_string (url);
  if (strncmp (mhdir, "mh:", 3) == 0)
    mhdir += 3;

  /* If no `msgs' are specified, mhpath outputs the folder pathname
     instead.  */
  if (argc == 0)
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
  
  if (argc == 1 && strcmp (argv[0], "new") == 0)
    {
      mu_message_t msg = NULL;
      size_t num;
      
      mu_mailbox_get_message (mbox, total, &msg);
      mh_message_number (msg, &num);
      printf ("%s/%s\n", mhdir, mu_umaxtostr (0, num + 1));
      exit (0);
    }
      
  /* Mhpath  expands  and  sorts  the  message  list `msgs' and
     writes the full pathnames of the messages to the  standard
     output separated by newlines. */
  mh_msgset_parse (&msgset, mbox, argc, argv, "cur");
  status = mu_msgset_foreach_message (msgset, mhpath, mhdir);
  mu_mailbox_close (mbox);
  mu_mailbox_destroy (&mbox);
  return status != 0;
}
