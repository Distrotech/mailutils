/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

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

/* Global variables and constants*/
mailbox_t mbox;
unsigned int cursor;
unsigned int realcursor;
unsigned int total;
FILE *ofile;

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
  {"to",      't', 0,      0, "Precede message by a list of addresses"},
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
    case ARGP_KEY_ARG:
      args->args = realloc (args->args,
			    sizeof (char *) * (state->arg_num + 2));
      args->args[state->arg_num] = arg;
      args->args[state->arg_num + 1] = NULL;
      util_do_command ("set mode=send");
      break;
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

static struct argp argp = { options, parse_opt, args_doc, doc };

static char *
mail_cmdline(void *closure, int cont)
{
  struct mail_env_entry *pev = closure;
  char *prompt = NULL;

  if (mail_is_terminal())
    prompt = pev->set && pev->value != NULL ? pev->value : "? ";
  return readline (prompt);
}

int
main (int argc, char **argv)
{
  struct mail_env_entry *mode = NULL, *prompt = NULL;
  int modelen = 0;
  struct arguments args;

  ofile = stdout;
  cursor = 1;
  realcursor = cursor;

  /* Register the desire formats.  */
  {
    list_t bookie;
    registrar_get_list (&bookie);
    list_append (bookie, mbox_record);
    list_append (bookie, path_record);
    list_append (bookie, pop_record);
    list_append (bookie, imap_record);
    /* Only use sendmail for mail ??  */
    list_append (bookie, sendmail_record);
  }

  signal (SIGPIPE, SIG_IGN);

  /* set up the default environment */
  if (!getenv ("HOME"))
    {
      char *p = util_get_homedir();
      setenv ("HOME", p, 0);
    }
  setenv ("DEAD", util_fullpath("~/dead.letter"), 0); 
  setenv ("EDITOR", "ed", 0);
  setenv ("LISTER", "ls", 0);
  setenv ("MAILRC", util_fullpath("~/.mailrc"), 0);
  setenv ("MBOX", util_fullpath("~/mbox"), 0);	
  setenv ("PAGER", "more", 0);
  setenv ("SHELL", "sh", 0);
  setenv ("VISUAL", "vi", 0);
  setenv ("COLUMNS", "80", 0);
  setenv ("LINES", "24", 0);

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

  args.args = NULL;
  args.file = NULL;
  args.user = NULL;

  /* argument parsing */
  argp_parse (&argp, argc, argv, 0, 0, &args);

  /* read .mailrc */
  if ((util_find_env ("rc"))->set)
    util_do_command ("source %s", getenv ("MAILRC"));

  /* how should we be running? */
  if ((mode = util_find_env ("mode")) == NULL || mode->set == 0)
    exit (EXIT_FAILURE);
  modelen = strlen (mode->value);

  /* Interactive mode */
  if (!(util_find_env("quiet"))->set)
    {
      fprintf (ofile, "%s, Copyright (C) 2001 Free Software Foundation, Inc.\n"
	       "mail comes with ABSOLUTELY NO WARRANTY; for details type\n"
	       "'warranty'. This is free software, and you are welcome to\n"
	       "redistribute it under certain conditions; type 'copying'\n"
	       "for details.\n",
	       argp_program_version);
    }

  /* Mode is just sending */
  if (strlen ("send") == modelen && !strcmp ("send", mode->value))
    {
      /* FIXME: set cmd to "mail [add1...]" */
      char *buf = NULL;
      int num = 0;
      if (args.args != NULL)
	while (args.args[num] != NULL)
	  num++;
      argcv_string (num, args.args, &buf);
      return util_do_command ("mail %s", buf);
    }
  /* Or acting as a normal reader */
  else
    {
      /* Initialize readline */
#ifdef WITH_READLINE
      rl_readline_name = "mail";
      rl_attempted_completion_function = (CPPFunction*)util_command_completion;
#endif

      /* open the mailbox */
      if (args.file == NULL)
	{
	  if (mailbox_create_default (&mbox, args.user) != 0)
	    exit (EXIT_FAILURE);
	}
      else if (mailbox_create (&mbox, args.file) != 0)
	exit (EXIT_FAILURE);

      if (mailbox_open (mbox, MU_STREAM_RDWR) != 0)
	exit (EXIT_FAILURE);

      if (mailbox_messages_count (mbox, &total) != 0)
	exit (EXIT_FAILURE);

      if (strlen ("exist") == modelen && !strcmp ("exist", mode->value))
	return (total < 1) ? 1 : 0;
      else if (strlen ("print") == modelen && !strcmp ("print", mode->value))
	return util_do_command ("print *");
      else if (strlen ("headers") == modelen
	       && !strcmp ("headers", mode->value))
	return util_do_command ("from *");

      /* initial commands */
      if ((util_find_env("header"))->set)
	  util_do_command ("z");

      prompt = util_find_env ("prompt");
      mail_set_is_terminal(isatty(0));
      mail_mainloop(mail_cmdline, (void*) prompt, 1);
      fprintf (ofile, "\n");
      util_do_command ("quit");
      return 0;
    }
  /* We should never reach this point */
  return 1;
}


void
mail_mainloop(char *(*input) __P((void *, int)), void *closure, int do_history)
{
  char *command, *cmd;
  while ((command = (*input)(closure, 0)) != NULL)
    {
      int len = strlen (command);
      while (command[len-1] == '\\')
	{
	  char *buf;
	  char *command2 = (*input) (closure, 1);

	  if (!command2)
	    {
	      command[len-1] = 0;
	      break;
	    }
	  command[len-1] = '\0';
	  buf = malloc ((len + strlen (command2)) * sizeof (char));
	  strcpy (buf, command);
	  strcat (buf, command2);
	  free (command);
	  command = buf;
	  len = strlen (command);
	}
      util_escape_percent (&command);
      cmd = util_stripwhite (command);
      util_do_command (cmd);
#ifdef WITH_READLINE
      if (do_history)
	add_history (cmd);
#endif
      if (command)
	free (command);
    }
}  
  


	
