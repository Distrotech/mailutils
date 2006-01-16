/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002, 2003, 2004,
   2005, 2006 Free Software Foundation, Inc.

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
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
   MA 02110-1301 USA */

#include "mail.h"

/* Global variables and constants*/
mu_mailbox_t mbox;               /* Mailbox being operated upon */
size_t total;                 /* Total number of messages in the mailbox */
FILE *ofile;                  /* Output file */
int interactive;              /* Is the session interactive */  

static mu_list_t command_list;   /* List of commands to be executed after parsing
				 command line */

const char *program_version = "mail (" PACKAGE_STRING ")";
static char doc[] = N_("GNU mail -- the standard /bin/mail interface");
static char args_doc[] = N_("[address...]");

static struct argp_option options[] = {
  {"exist",   'e', 0,      0, N_("Return true if mail exists"), 0},
  {"file",    'f', N_("URL"), OPTION_ARG_OPTIONAL,
			      N_("Operate on given mailbox URL (default ~/mbox)"), 0},
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
  {"append",  'a', N_("HEADER: VALUE"), 0,
   N_("Append given header to the message being sent"), 0},
  {"exec",    'E', N_("COMMAND"), 0,
   N_("Execute COMMAND"), 0 },
  { NULL,      0, NULL, 0, NULL, 0 }
};


struct arguments
{
  char **args;
  char *file;
  char *user;
  int send_mode;
};

static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  struct arguments *args = state->input;

  switch (key)
    {
    case 'a':
      args->send_mode = 1;
      send_append_header (arg);
      break;
      
    case 'e':
      util_cache_command (&command_list, "set mode=exist");
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
      util_cache_command (&command_list, "set mode=print");
      break;
      
    case 'q':
      util_cache_command (&command_list, "set quit");
      break;
      
    case 't':
      util_cache_command (&command_list, "set mode=send");
      break;
      
    case 'H':
      util_cache_command (&command_list, "set mode=headers");
      break;
      
    case 'i':
      util_cache_command (&command_list, "set ignore");
      break;
      
    case 'n':
      util_do_command ("set norc");
      break;
      
    case 'N':
      util_cache_command (&command_list, "set noheader");
      break;
      
    case 's':
      send_append_header2 (MU_HEADER_SUBJECT, arg, COMPOSE_REPLACE);
      util_cache_command (&command_list, "set noasksub");
      args->send_mode = 1;
      break;
      
    case 'u':
      args->user = arg;
      break;

    case 'E':
      util_cache_command (&command_list, arg);
      break;
      
    case 'F':
      util_cache_command (&command_list, "set byname");
      break;

    case ARGP_KEY_ARG:
      /* People often tend to separate -f option from its argument
	 with a whitespace. This heuristics tries to catch the
	 error: */

      if (args->file)
	{
	  args->file = arg;
	}
      else
	{
	  args->args = realloc (args->args,
				sizeof (char *) * (state->arg_num + 2));
	  args->args[state->arg_num] = arg;
	  args->args[state->arg_num + 1] = NULL;
	  util_cache_command (&command_list, "set mode=send");
	}
      break;

    case ARGP_KEY_FINI:
      if (args->send_mode)
	util_cache_command (&command_list, "set mode=send");
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
#ifdef WITH_TLS
	"tls",
#endif
	 NULL 
};
			     
static char *
mail_cmdline(void *closure, int cont ARG_UNUSED)
{
  char *prompt = (char*) closure;
  char *rc;

  while (1)
    {
      if (util_getenv (NULL, "autoinc", Mail_env_boolean, 0) == 0
          && !mu_mailbox_is_updated (mbox))
	{
	  mu_mailbox_messages_count (mbox, &total);
	  page_invalidate (0);
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

static char *default_setup[] = {
  "set noallnet",
  "set noappend",
  "set asksub",
  "set crt",
  "set noaskbcc",
  "set askcc",
  "set noautoprint",
  "set nobang",
  "set nocmd",
  "set nodebug",
  "set nodot",
  "set escape=~",
  "set noflipr",
  "set nofolder",
  "set header",
  "set nohold",
  "set noignore",
  "set noignoreeof",
  "set indentprefix=\"\t\"",
  "set nokeep",
  "set nokeepsave",
  "set nometoo",
  "set noonehop",
  "set nooutfolder",
  "set nopage",
  "set prompt=\"? \"",
  "set norecord",
  "set save",
  "set nosendmail",
  "set nosendwait",
  "set noshowto",
  "set nosign",
  "set noSign",
  "set toplines=5",
  "set autoinc",
  "set regex",
  "set replyprefix=\"Re: \"",
  "set charset=auto",
  "set xmailer",
  "unfold subject",
  "sender mail-followup-to reply-to from",
  "set nocmd",
  "set metamail",
  "set recursivealiases",
  "set noinplacealiases",
  
  /* Start in mail reading mode */
  "set mode=read",
  "set noquit",
  "set rc",

  "set noflipr",
  "set noshowto",
  "set nobang",
  
  /* These settings are not yet used */
  "set nodebug",
  "set noonehop",
  "set nosendwait",
};

int
main (int argc, char **argv)
{
  char *mode = NULL, *prompt = NULL;
  struct arguments args;
  int i, rc;
  
  ofile = stdout;
  set_cursor (1);

  /* Native Language Support */
  mu_init_nls ();

  /* Register the desired formats.  */
  mu_register_all_formats ();

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
      char *p = util_get_homedir ();
      setenv ("HOME", p, 0);
    }

  /* Set up the default environment */
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
  for (i = 0; i < sizeof(default_setup)/sizeof(default_setup[0]); i++)
    util_do_command (default_setup[i]);
  util_do_command ("set screen=%d", util_getlines ());
  util_do_command ("set columns=%d", util_getcols ());
  
  /* Set the default mailer to sendmail.  */
  {
    char *mailer_name = alloca (strlen ("sendmail:")
				+ strlen (_PATH_SENDMAIL) + 1);
    sprintf (mailer_name, "sendmail:%s", _PATH_SENDMAIL);
    util_setenv ("sendmail", mailer_name, Mail_env_string, 1);
  }


  args.args = NULL;
  args.file = NULL;
  args.user = NULL;
  args.send_mode = 0;
  
  /* argument parsing */

  mu_argp_init (program_version, NULL);
#ifdef WITH_TLS
  mu_tls_init_client_argp ();
#endif
  mu_argp_parse (&argp, &argc, &argv, 0, mail_capa, NULL, &args);

  /* read system-wide mail.rc and user's .mailrc */
  if (util_getenv (NULL, "rc", Mail_env_boolean, 0) == 0)
    util_do_command ("source %s", SITE_MAIL_RC);
  util_do_command ("source %s", getenv ("MAILRC"));

  util_run_cached_commands (&command_list);

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

  /* Interactive mode */

  ml_readline_init ();
  mail_set_my_name(args.user);

  /* Mode is just sending */
  if (strcmp (mode, "send") == 0)
    {
      /* FIXME: set cmd to "mail [add1...]" */
      char *buf = NULL;
      int num = 0;
      int rc;
      if (args.args != NULL)
	while (args.args[num] != NULL)
	  num++;
      mu_argcv_string (num, args.args, &buf);
      rc = util_do_command ("mail %s", buf);
      return util_getenv (NULL, "mailx", Mail_env_boolean, 0) ? rc : 0;
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
	      rc = mu_mailbox_create_default (&mbox, p);
	      free (p);
	    }
	  else
	    rc = mu_mailbox_create_default (&mbox, NULL);
	  if (rc != 0)
	    {
	      util_error (_("Cannot create mailbox for %s: %s"), args.user,
			  mu_strerror (rc));
	      exit (EXIT_FAILURE);
	    }
	}
      else if ((rc = mu_mailbox_create_default (&mbox, args.file)) != 0)
	{
	  util_error (_("Cannot create mailbox %s: %s"), args.file,
		      mu_strerror (rc));
	  exit (EXIT_FAILURE);
	}

      /* Could we enable this at runtime, via the a set environment?  */
      if (0)
	{
	  mu_debug_t debug = NULL;
	  mu_mailbox_get_debug (mbox, &debug);
	  mu_debug_set_level (debug, MU_DEBUG_TRACE|MU_DEBUG_PROT);
	}

      if ((rc = mu_mailbox_open (mbox, MU_STREAM_RDWR)) != 0 && rc != ENOENT)
	{
	  mu_url_t url = NULL;
	  mu_mailbox_get_url (mbox, &url);
	  util_error (_("Cannot open mailbox %s: %s"),
		      mu_url_to_string (url), mu_strerror (rc));
	  mu_mailbox_destroy (&mbox);
	  exit (EXIT_FAILURE);
	}

      if (rc)
	total = 0;
      else
	{
	  if ((rc = mu_mailbox_scan (mbox, 1, &total)) != 0)
	    {
	      mu_url_t url = NULL;
	      mu_mailbox_get_url (mbox, &url);
	      util_error (_("Cannot read mailbox %s: %s"),
			  mu_url_to_string (url), mu_strerror (rc));
	      exit (EXIT_FAILURE);
	    }

	  if (strcmp (mode, "exist") == 0)
	    return (total < 1) ? 1 : 0;
	  else if (strcmp (mode, "print") == 0)
	    return util_do_command ("print *");
	  else if (strcmp (mode, "headers") == 0)
	    return util_do_command ("from *");
	  else if (strcmp (mode, "read"))
	    {
	      util_error(_("Unknown mode `%s'"), mode);
	      util_do_command ("quit");
	      return 1;
	    }
	}
      
      if (total == 0
	  && (strcmp (mode, "read")
	      || util_getenv (NULL, "emptystart", Mail_env_boolean, 0)))
        {
	  if (args.file)
	    fprintf (ofile, _("%s: 0 messages\n"), args.file);
	  else
	    fprintf (ofile, _("No mail for %s\n"),
		     args.user ? args.user : mail_whoami ());
          return 1;
        }

      /* initial commands */
      if (util_getenv (NULL, "header", Mail_env_boolean, 0) == 0)
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
mail_mainloop (char *(*input) (void *, int),
	       void *closure, int do_history)
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
mail_warranty (int argc ARG_UNUSED, char **argv ARG_UNUSED)
{
  fputs (_("GNU Mailutils -- a suite of utilities for electronic mail\n"
           "Copyright (C) 1999, 2000, 2001, 2002, 2003, 2004, 2005 Free Software Foundation, Inc.\n\n"),
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
    "   You should have received a copy of the GNU General Public License along\n"
    "   with GNU Mailutils; if not, write to the Free Software Foundation,\n"
    "   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA\n"
    "\n"
    "\n"
),
    ofile);

  return 0;
}

