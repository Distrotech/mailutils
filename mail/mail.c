/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002 Free Software Foundation, Inc.

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

#include "mail.h"

/* Global variables and constants*/
mailbox_t mbox;
unsigned int cursor;
size_t total;
FILE *ofile;
int interactive;

const char *argp_program_version = "mail (" PACKAGE_STRING ")";
static char doc[] = N_("GNU mail -- the standard /bin/mail interface");
static char args_doc[] = N_("[address...]");

static struct argp_option options[] = {
  {"exist",   'e', 0,      0, N_("Return true if mail exists"), 0},
  {"file",    'f', N_("FILE"), OPTION_ARG_OPTIONAL,
			          N_("Operate on mailbox FILE (default ~/mbox)"), 0},
  {"byname",  'F', 0,      0, N_("Save messages according to sender"), 0},
  {"headers", 'H', 0,      0, N_("Write a header summary and exit"), 0},
  {"ignore",  'i', 0,      0, N_("Ignore interrupts"), 0},
  {"norc",    'n', 0,      0, N_("Do not read the system mailrc file"), 0},
  {"nosum",   'N', 0,      0, N_("Do not display initial header summary"), 0},
  {"print",   'p', 0,      0, N_("Print all mail to standard output"), 0},
  {"quit",    'q', 0,      0, N_("Cause interrupts to terminate program"), 0},
  {"read",    'r', 0,      0, N_("Same as -p"), 0},
  {"subject", 's', N_("SUBJ"), 0, N_("Send a message with a Subject of SUBJ"), 0},
  {"to",      't', 0,      0, N_("Precede message by a list of addresses"), 0},
  {"user",    'u', N_("USER"), 0, N_("Operate on USER's mailbox"), 0},
  {NULL,      0,  NULL,    OPTION_DOC,
   N_("Note: Argument to --file (-f) option is optional. If it is present, "
   "it must follow the short option immediately, without any intervening "
   "whitespace. If it is used with the long option, it must be separated "
   "from it by an equal sign, with no intervening whitespace."), 0},
  { NULL,      0, NULL, 0, NULL, 0 }
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
	  args->file = xmalloc(len * sizeof (char));
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
      /* People often tend to separate -f option from its argument
	 with a whitespace. This heuristics tries to catch the
	 error: */

      if (args->file)
	{
	  util_error (_("Usage error: --file takes an optional argument, "
	              "it must follow the option\n"
	              "without any intervening whitespace."));
	  util_error (_("Run mail --help for more info."));
	  args->file = arg;
	}
      else
	{
	  args->args = realloc (args->args,
				sizeof (char *) * (state->arg_num + 2));
	  args->args[state->arg_num] = arg;
	  args->args[state->arg_num + 1] = NULL;
	  util_do_command ("set mode=send");
	}
      break;
      
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

static struct argp argp = {
  options,
  parse_opt,
  args_doc,
  doc,
  NULL,
  NULL, NULL
};

static const char *mail_capa[] = {
	"common",
	"license",
	"mailbox",
	 NULL 
};
			     
static char *
mail_cmdline(void *closure, int cont)
{
  char *prompt = (char*) closure;
  char *rc;

  (void)cont;

  while (1)
    {
      if (util_getenv (NULL, "autoinc", Mail_env_boolean, 0) == 0
          && !mailbox_is_updated (mbox))
	{
	  mailbox_messages_count (mbox, &total);
	  fprintf (ofile, _("New mail has arrived.\n"));
	}

      rc = ml_readline (prompt);

      if (ml_got_interrupt())
	{
	  util_error(_("Interrupt"));
	  continue;
	}

      if (!rc && util_getenv (NULL, "ignoreeof", Mail_env_boolean, 0) == 0)
	{
	  util_error (_("Use \"quit\" to quit."));
	  continue;
	}

      break;
    }
  return rc;
}

int
main (int argc, char **argv)
{
  char *mode = NULL, *prompt = NULL;
  size_t modelen = 0;
  struct arguments args;
  int rc;
  
  ofile = stdout;
  cursor = 1;

  /* Native Language Support */
  mu_init_nls ();

  /* Register the desire formats.  */
  {
    list_t bookie;
    registrar_get_list (&bookie);
    list_append (bookie, mbox_record);
    list_append (bookie, path_record);
    list_append (bookie, pop_record);
    list_append (bookie, imap_record);
    list_append (bookie, mh_record);
    /* Possible supported mailers.  */
    list_append (bookie, sendmail_record);
    list_append (bookie, smtp_record);
  }

  interactive = isatty (fileno(stdin));
#ifdef HAVE_SIGACTION
  {
    struct sigaction act;
    act.sa_handler = SIG_IGN;
    sigemptyset (&act.sa_mask);
    act.sa_flags = 0;
    sigaction (SIGPIPE, &act, NULL);
  }
#else
  signal (SIGPIPE, SIG_IGN);
#endif

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
  util_do_command ("set asksub");
  util_do_command ("set crt");
  util_do_command ("set noaskbcc");
  util_do_command ("set askcc");
  util_do_command ("set noautoprint");
  util_do_command ("set nobang");
  util_do_command ("set nocmd");
  util_do_command ("set nodebug");
  util_do_command ("set nodot");
  util_do_command ("set escape=~");
  util_do_command ("set noflipr");
  util_do_command ("set nofolder");
  util_do_command ("set header");
  util_do_command ("set nohold");
  util_do_command ("set noignore");
  util_do_command ("set noignoreeof");
  util_do_command ("set indentprefix=\"\t\"");
  util_do_command ("set nokeep");
  util_do_command ("set nokeepsave");
  util_do_command ("set nometoo");
  util_do_command ("set noonehop");
  util_do_command ("set nooutfolder");
  util_do_command ("set nopage");
  util_do_command ("set prompt=\"? \"");
  util_do_command ("set norecord");
  util_do_command ("set save");
  util_do_command ("set screen=%d", util_getlines ());
  util_do_command ("set columns=%d", util_getcols ());
  util_do_command ("set nosendmail");
  util_do_command ("set nosendwait");
  util_do_command ("set noshowto");
  util_do_command ("set nosign");
  util_do_command ("set noSign");
  util_do_command ("set toplines=5");
  util_do_command ("set autoinc");
  util_do_command ("set regex");
  
  /* Set the default mailer to sendmail.  */
  {
    char *mailer_name = alloca (strlen ("sendmail:")
				+ strlen (_PATH_SENDMAIL) + 1);
    sprintf (mailer_name, "sendmail:%s", _PATH_SENDMAIL);
    util_setenv ("sendmail", mailer_name, Mail_env_string, 1);
  }

  /* GNU extensions to the environment, for sparky's sanity */
  util_do_command ("set mode=read");
  util_do_command ("set nobyname");
  util_do_command ("set rc");
  util_do_command ("set noquit");

  args.args = NULL;
  args.file = NULL;
  args.user = NULL;

  /* argument parsing */

  mu_argp_parse (&argp, &argc, &argv, 0, mail_capa, NULL, &args);

  /* read system-wide mail.rc and user's .mailrc */
  if (util_getenv (NULL, "rc", Mail_env_boolean, 0) == 0)
    util_do_command ("source %s", SITE_MAIL_RC);
  util_do_command ("source %s", getenv ("MAILRC"));
  if (!interactive)
    {
      util_do_command ("set nocrt");
      util_do_command ("set noasksub");
      util_do_command ("set noaskcc");
      util_do_command ("set noaskbcc");
    }

  /* how should we be running? */
  if (util_getenv (&mode, "mode", Mail_env_string, 1))
    exit (EXIT_FAILURE);
  modelen = strlen (mode);

  /* Interactive mode */

  ml_readline_init ();
  mail_set_my_name(args.user);

  /* Mode is just sending */
  if (strlen ("send") == modelen && !strcmp ("send", mode))
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
      /* open the mailbox */
      if (args.file == NULL)
	{
	  if (args.user)
	    {
	      char *p = xmalloc (strlen (args.user) + 2);
	      p[0] = '%';
	      strcpy (p + 1, args.user);
	      rc = mailbox_create_default (&mbox, p);
	      free (p);
	    }
	  else
	    rc = mailbox_create_default (&mbox, NULL);
	  if (rc != 0)
	    {
	      util_error (_("Can not create mailbox for %s: %s"), args.user,
			  mu_errstring (rc));
	      exit (EXIT_FAILURE);
	    }
	}
      else if ((rc = mailbox_create_default (&mbox, args.file)) != 0)
	{
	  util_error (_("Can not create mailbox %s: %s"), args.file,
		      mu_errstring (rc));
	  exit (EXIT_FAILURE);
	}

      /* Could we enable this at runtime, via the a set environment?  */
      if (0)
	{
	  mu_debug_t debug = NULL;
	  mailbox_get_debug (mbox, &debug);
	  mu_debug_set_level (debug, MU_DEBUG_TRACE|MU_DEBUG_PROT);
	}

      if ((rc = mailbox_open (mbox, MU_STREAM_RDWR)) != 0 && rc != ENOENT)
	{
	  url_t url = NULL;
	  mailbox_get_url (mbox, &url);
	  util_error (_("Can not open mailbox %s: %s"),
		      url_to_string (url), mu_errstring (rc));
	  exit (EXIT_FAILURE);
	}

      if (rc)
	total = 0;
      else
	{
	  if ((rc = mailbox_scan (mbox, 1, &total)) != 0)
	    {
	      url_t url = NULL;
	      mailbox_get_url (mbox, &url);
	      util_error (_("Can not read mailbox %s: %s"),
			  url_to_string (url), mu_errstring (rc));
	      exit (EXIT_FAILURE);
	    }

	  if (strlen ("exist") == modelen && !strcmp ("exist", mode))
	    return (total < 1) ? 1 : 0;
	  else if (strlen ("print") == modelen
		   && !strcmp ("print", mode))
	    return util_do_command ("print *");
	  else if (strlen ("headers") == modelen
		   && !strcmp ("headers", mode))
	    return util_do_command ("from *");
	}
      
      if (total == 0)
        {
	  if (args.file)
	    fprintf (ofile, _("%s: 0 messages\n"), args.file);
	  else
	    fprintf (ofile, _("No mail for %s\n"),
		     args.user ? args.user : mail_whoami ());
          return 1;
        }

      /* initial commands */
      if (util_getenv(NULL, "header", Mail_env_boolean, 0) == 0)
	{
	  util_do_command ("summary");
	  util_do_command ("z.");
	}

      util_getenv (&prompt, "prompt", Mail_env_string, 0);
      mail_mainloop (mail_cmdline, (void*) prompt, 1);
      fprintf (ofile, "\n");
      util_do_command ("quit");
      return 0;
    }
  /* We should never reach this point */
  return 1;
}


void
mail_mainloop (char *(*input) __P((void *, int)), void *closure, int do_history)
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
	  buf = xmalloc ((len + strlen (command2)) * sizeof (char));
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
      if (do_history && !(isspace(cmd[0]) || cmd[0] == '#'))
	add_history (cmd);
#endif
      if (command)
	free (command);
    }
}

int
mail_warranty (int argc, char **argv)
{
  (void)argc; (void)argv;

  fputs (_("GNU Mailutils -- a suite of utilities for electronic mail\n"
           "Copyright (C) 1999, 2000, 2001, 2002 Free Software Foundation, Inc.\n\n"),
           ofile);
  fputs (
  _("   GNU Mailutils is free software; you can redistribute it and/or modify\n"
    "   it under the terms of the GNU General Public License as published by\n"
    "   the Free Software Foundation; either version 2 of the License, or\n"
    "   (at your option) any later version.\n"
    "\n"
    "   GNU Mailutils is distributed in the hope that it will be useful,\n"
    "   but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
    "   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
    "   GNU General Public License for more details.\n"
    "\n"
    "   You should have received a copy of the GNU General Public License\n"
    "   along with GNU Mailutils; if not, write to the Free Software\n"
    "   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA\n\n"),
    ofile);

  return 0;
}
