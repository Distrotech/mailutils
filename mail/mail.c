/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000 Free Software Foundation, Inc.

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
  {"byname",  'F', 0,      0, "Save messages according to sender"},
  {"headers", 'H', 0,      0, "Write a header summary and exit"},
  {"ignore",  'i', 0,      0, "Ignore interrupts"},
  {"norc",    'n', 0,      0, "Do not read the system mailrc file"},
  {"nosum",   'N', 0,      0, "Do not display initial header summary"},
  {"print",   'p', 0,      0, "Print all mail to standard output"},
  {"quit",    'q', 0,      0, "Cause interrupts to terminate program"},
  {"read",    'r', 0,      0, "Same as -p"},
  {"subject", 's', "SUBJ", 0, "Send a message with a Subject of SUBJ"},
  {"to",      't', 0,      0, "Preced message by a list of addresses"},
  {"user",    'u', "USER", 0, "Operate on USER's mailbox"},
  { 0 }
};

struct arguments
{
  char **args;
  char *file;
  char *user;
};

static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  struct arguments *args = state->input;

  switch (key)
    {
    case 'e':
      util_do_command ("set mode=exist");
      break;
    case 'f':
      if (arg != NULL)
	args->file = arg;
      else
	{
	  int len;
	  char *home = getenv("HOME");
	  if (home == NULL)
	    {
	      fprintf (stderr, "No Home!\n");
	      home = (char *)"";
	    }
	  len = strlen (home) + strlen ("/mbox") + 1;
	  args->file = malloc(len * sizeof (char));
	  strcpy (args->file, home);
	  strcat (args->file, "/mbox");
	}
      break;
    case 'p':
    case 'r':
      util_do_command ("set mode=print");
      break;
    case 'q':
      util_do_command ("set quit");
      break;
    case 't':
      util_do_command ("set mode=send");
      break;
    case 'H':
      util_do_command ("set mode=headers");
      break;
    case 'i':
      util_do_command ("set ignore");
      break;
    case 'n':
      util_do_command ("set norc");
      break;
    case 'N':
      util_do_command ("set noheader");
      break;
    case 's':
      util_do_command ("set mode=send");
      util_do_command ("set noasksub");
      util_do_command ("set subject=\"%s\"", arg);
      break;
    case 'u':
      args->user = arg;
      break;
    case 'F':
      util_do_command ("set byname");
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
  char *command = NULL, *cmd = NULL;
  struct mail_env_entry *mode = NULL, *prompt = NULL;
  int modelen = 0;
  struct arguments args;

  cursor = 1;
  realcursor = cursor;

  signal (SIGPIPE, SIG_IGN);

  /* set defaults for execution */
  util_do_command ("set noallnet");
  util_do_command ("set noappend");
  if (isatty (fileno(stdin))) { util_do_command ("set asksub"); }
  else { util_do_command ("set noasksub"); }
  util_do_command ("set noaskbcc");
  util_do_command ("set askcc");
  util_do_command ("set noautoprint");
  util_do_command ("set nobang");
  util_do_command ("set nocmd");
  util_do_command ("set nocrt");
  util_do_command ("set nodebug");
  util_do_command ("set nodot");
  util_do_command ("set escape=~");
  util_do_command ("set noflipr");
  util_do_command ("set nofolder");
  util_do_command ("set header");
  util_do_command ("set nohold");
  util_do_command ("set noignore");
  util_do_command ("set noignoreeof");
  util_do_command ("set indentprefix=\t");
  util_do_command ("set nokeep");
  util_do_command ("set nokeepsave");
  util_do_command ("set nometoo");
  util_do_command ("set noonehop");
  util_do_command ("set nooutfolder");
  util_do_command ("set nopage");
  util_do_command ("set prompt=?"); /* FIXME: "set prompt=\"? \"" */
  util_do_command ("set noquiet");
  util_do_command ("set norecord");
  util_do_command ("set save");
  util_do_command ("set screen=%d", util_getlines());
  util_do_command ("set nosendmail");
  util_do_command ("set nosendwait");
  util_do_command ("set noshowto");
  util_do_command ("set nosign");
  util_do_command ("set noSign");
  util_do_command ("set toplines=5");

  /* GNU extensions to the environment, for sparky's sanity */
  util_do_command ("set mode=read");
  util_do_command ("set nobyname");
  util_do_command ("set rc");
  util_do_command ("set noquit");

  args.file = NULL;
  args.user = NULL;

  /* argument parsing */
  argp_parse (&argp, argc, argv, 0, 0, &args);

  /* Initialize readline */
  rl_readline_name = "mail";
  rl_attempted_completion_function = (CPPFunction *)util_command_completion;

  /* open the mailbox */
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

  /* how should we be running? */
  if ((mode = util_find_env ("mode")) == NULL || mode->set == 0)
    exit (EXIT_FAILURE);

  modelen = strlen (mode->value);
  if (strlen ("exist") == modelen && !strcmp ("exist", mode->value))
    return (total < 1) ? 1 : 0;
  else if (strlen ("print") == modelen && !strcmp ("print", mode->value))
    return util_do_command ("print *");
  else if (strlen ("headers") == modelen && !strcmp ("headers", mode->value))
    return util_do_command ("from *");
  else if (strlen ("send") == modelen && !strcmp ("send", mode->value))
    {
      /* set cmd to "mail [add1...]" */
      cmd = strdup ("mail");
      return util_do_command (cmd);
    }

  /* initial commands */
  if ((util_find_env("header"))->set)
    util_do_command ("from *");

  prompt = util_find_env ("prompt");

  while (1)
    {
      int len;
      free (command);
      command = readline (prompt->set && prompt->value != NULL ? prompt->value : "");
      len = strlen (command);
      while (command[len-1] == '\\')
	{
	  char *buf;
	  char *command2 = readline ("> ");

	  command[len-1] = '\0';
	  buf = malloc ((len + strlen (command2)) * sizeof (char));
	  strcpy (buf, command);
	  strcat (buf, command2);
	  free (command);
	  command = buf;
	  len = strlen (command);
	}
      cmd = util_stripwhite (command);
      util_do_command (cmd);
      add_history (cmd);
    }
}
