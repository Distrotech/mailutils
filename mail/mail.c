/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999 Free Software Foundation, Inc.

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

#include "mail.h"

const char *argp_program_version = "mail (" PACKAGE ") " VERSION;
const char *argp_program_bug_address = "<bug-mailutils@gnu.org>";
static char doc[] = "GNU mail -- the standard /bin/mail interface";
static char args_doc[] = "[address...]";

static struct argp_option options[] = {
  {"exist",   'e', 0,      0, "Return true if mail exists"},
  {"file",    'f', "FILE", OPTION_ARG_OPTIONAL,
			      "Operate on mailbox FILE (default ~/mbox)"},
  {"FIXME",   'F', 0,      0, "Save messages according to sender"},
  {"headers", 'H', 0,      0, "Write a header summary and exit"},
  {"ignore",  'i', 0,      0, "Ignore interrupts"},
  {"norc",    'n', 0,      0, "Do not read the system mailrc file"},
  {"nosum",   'N', 0,      0, "Do not display initial header summary"},
  {"print",   'p', 0,      0, "Print all mail to standard out"},
  {"quit",    'q', 0,      0, "Cause interrupts to terminate program"},
  {"read",    'r', 0,      0, "Write messages in FIFO order"},
  {"subject", 's', "SUBJ", 0, "Send a message with a Subject of SUBJ"},
  {"to",      't', 0,      0, "Preced message by a list of addresses"},
  {"user",    'u', "USER", 0, "Operate on USER's mailbox"},
  { 0 }
};

struct arguments
{
  char **args;
  int exist, print, quit, read, to, header, ignore, norc, nosum;
  char *file;
  char *subject;
  char *user;
};

static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  struct arguments *args = state->input;

  switch (key)
    {
    case 'e':
      args->exist = 1;
      break;
    case 'f':
      if (arg != NULL)
	args->file = arg;
      else
	{
	  char *home = getenv("HOME");
	  int len = strlen (home) + strlen ("/mbox") + 1;
	  args->file = malloc(len * sizeof (char));
	  strcpy (args->file, home);
	  strcat (args->file, "/mbox");
	  free (home);
	}
      break;
    case 'p':
      args->print = 1;
      break;
    case 'q':
      args->quit = 1;
      break;
    case 'r':
      args->read = 1;
      break;
    case 't':
      args->to = 1;
      break;
    case 'H':
      args->header = 1;
      break;
    case 'i':
      args->ignore = 1;
      break;
    case 'n':
      args->norc = 1;
      break;
    case 'N':
      args->nosum = 1;
      break;
    case 's':
      args->subject = arg;
      break;
    case 'u':
      args->user = arg;
      break;
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

static struct argp argp = { options, parse_opt, args_doc, doc };

int
main (int argc, char **argv)
{
  char *command = NULL;
  struct arguments args;

  cursor = 1;

  args.exist = 0;
  args.print = 0;
  args.quit = 0;
  args.read = 0;
  args.to = 0;
  args.header = 0;
  args.ignore = 0;
  args.norc = 0;
  args.nosum = 0;
  args.file = NULL;
  args.subject = NULL;
  args.user = NULL;
  
  argp_parse (&argp, argc, argv, 0, 0, &args);

  if (args.file == NULL)
    {
      if (mailbox_create_default (&mbox, args.user) != 0)
	exit (EXIT_FAILURE);
    }
  else if (mailbox_create (&mbox, args.file, 0) != 0)
    exit (EXIT_FAILURE);
  
  if (mailbox_open (mbox, MU_STREAM_READ) != 0)
    exit (EXIT_FAILURE);

  if (mailbox_messages_count (mbox, &total) != 0)
    exit (EXIT_FAILURE);

  if (args.exist)
    {
      if (total < 1)
	exit (1);
      else
	exit (0);
    }

  while (1)
    {
      free (command);
      command = readline ("mail> ");
      util_do_command (command);
      add_history (command);
    }
}

