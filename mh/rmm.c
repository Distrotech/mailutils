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

/* MH rmm command */

#include <mh.h>

const char *argp_program_version = "rmm (" PACKAGE_STRING ")";
static char doc[] = "GNU MH rmm";
static char args_doc[] = "";

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

int
member (size_t num, size_t *msglist, size_t msgcnt)
{
  size_t i;

  for (i = 0; i < msgcnt; i++)
    if (msglist[i] == num)
      return 1;
  return 0;
}

int
rmm (mailbox_t mbox, size_t msgcnt, size_t *msglist)
{
  size_t i, total = 0;

  mailbox_messages_count (mbox, &total);
  for (i = 1; i <= total; i++)
    {
      message_t msg;
      size_t num;
      int rc;
      
      if ((rc = mailbox_get_message (mbox, i, &msg)) != 0)
	{
	  mh_error ("can't get message %d: %s", i, mu_errstring (rc));
	  return 1;
	}

      if ((rc = mh_message_number (msg, &num)) != 0)
	{
	  mh_error ("can't get sequence number for message %d: %s",
		    i, mu_errstring (rc));
	  return 1;
	}

      if (member (num, msglist, msgcnt))
	{
	  attribute_t attr;
	  message_get_attribute (msg, &attr);
	  attribute_set_deleted (attr);
	}
    }

  return 0;
}

int
main (int argc, char **argv)
{
  int index = 0;
  mailbox_t mbox;
  size_t msgcnt = 0, *msglist = NULL;
  int status;
  
  mh_argp_parse (argc, argv, options, mh_option, args_doc, doc,
		 opt_handler, NULL, &index);

  mbox = mh_open_folder ();
  
  if (index < argc)
    {
      size_t i;
      
      msgcnt = argc - index + 1;
      msglist = calloc (argc - index + 1, sizeof(*msglist));
      for (i = 0; index < argc; index++, i++)
	{
	  char *p = NULL;
	  msglist[i] = strtol (argv[index], &p, 0);
	  if (msglist[i] <= 0 || *p)
	    {
	      mh_error ("bad message list `%s'", argv[index]);
	      exit (1);
	    }
	}
    }
  else
    {
      if (current_message == 0)
	{
	  mh_error ("no cur message");
	  exit (1);
	}
      msglist = calloc (1, sizeof(*msglist));
      msglist[0] = current_message;
      current_message = 0;
      msgcnt = 1;
    }

  status = rmm (mbox, msgcnt, msglist);

  mailbox_expunge (mbox);
  mailbox_close (mbox);
  mailbox_destroy (&mbox);
  return status;
}

  
      
