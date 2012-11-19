/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2007, 2009-2012 Free Software Foundation, Inc.

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

#include "mail.h"
#include "mailutils/libargp.h"

/* Global variables and constants*/
mu_mailbox_t mbox;            /* Mailbox being operated upon */
size_t total;                 /* Total number of messages in the mailbox */
int interactive;              /* Is the session interactive */  

static mu_list_t command_list;   /* List of commands to be executed after parsing
				 command line */

const char *program_version = "mail (" PACKAGE_STRING ")";
static char doc[] = N_("GNU mail -- process mail messages.\n"
"If -f or --file is given, mail operates on the mailbox named "
"by the first argument, or the user's mbox, if no argument given.\n");
static char args_doc[] = N_("[address...]\n-f [OPTION...] [file]\n--file [OPTION...] [file]\n--file=file [OPTION...]");

#define F_OPTION       256
#define F_ENCODING     257
#define F_CONTENT_TYPE 258

static struct argp_option options[] = {
  { NULL,     'f', NULL,      OPTION_HIDDEN, NULL, 0 },
  {"file",    F_OPTION, "FILE",   OPTION_ARG_OPTIONAL|OPTION_HIDDEN, 0},

  {"exist",   'e', NULL,      0, N_("return true if mail exists"), 0},
  {"byname",  'F', NULL,      0, N_("save messages according to sender"), 0},
  {"headers", 'H', NULL,      0, N_("write a header summary and exit"), 0},
  {"ignore",  'i', NULL,      0, N_("ignore interrupts"), 0},
  {"norc",    'n', NULL,      0, N_("do not read the system mailrc file"), 0},
  {"nosum",   'N', NULL,      0,
   N_("do not display initial header summary"), 0},
  {"print",   'p', NULL,      0, N_("print all mail to standard output"), 0},
  {"read",    0,   NULL,      OPTION_ALIAS },
  {"return-address", 'r', N_("ADDRESS"), 0,
   N_("use address as the return address when sending mail"), 0},
  {"quit",    'q', NULL,      0,
   N_("cause interrupts to terminate program"), 0},
  {"subject", 's', N_("SUBJ"), 0,
   N_("send a message with the given SUBJECT"), 0},
  {"to",      't', NULL,      0,
   N_("precede message by a list of addresses"), 0},
  {"user",    'u', N_("USER"), 0, N_("operate on USER's mailbox"), 0},
  {"append",  'a', N_("HEADER: VALUE"), 0,
   N_("append given header to the message being sent"), 0},
  {"exec",    'E', N_("COMMAND"), 0,
   N_("execute COMMAND"), 0 },
  {"encoding", F_ENCODING, N_("NAME"), 0,
   N_("set encoding for subsequent --attach options"), 0 },
  {"content-type", F_CONTENT_TYPE, N_("TYPE"), 0,
   N_("set content type for subsequent --attach options"), 0 },
  {"attach",  'A', N_("FILE"), 0,
   N_("attach FILE"), 0 },
  { NULL,      0, NULL, 0, NULL, 0 }
};


#define HINT_SEND_MODE   0x1
#define HINT_FILE_OPTION 0x2

struct arguments
{
  int argc;
  char **argv;
  char *file;
  char *user;
  int hint;
};

static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  struct arguments *args = state->input;

  switch (key)
    {
    case 'a':
      args->hint |= HINT_SEND_MODE;
      send_append_header (arg);
      break;

    case 'A':
      args->hint |= HINT_SEND_MODE;
      if (send_attach_file_default (arg))
	exit (1);
      break;
      
    case F_CONTENT_TYPE:
      free (default_content_type);
      default_content_type = mu_strdup (arg);
      break;
      
    case 'e':
      util_cache_command (&command_list, "setq mode=exist");
      break;

    case F_OPTION:
      if (arg)
	args->file = arg;
      /* fall through */
    case 'f':
      args->hint |= HINT_FILE_OPTION;
      break;
      
    case 'p':
      util_cache_command (&command_list, "setq mode=print");
      break;
      
    case 'r':
      util_cache_command (&command_list, "set return-address=%s", arg);
      break;
      
    case 'q':
      util_cache_command (&command_list, "set quit");
      break;
      
    case 't':
      util_cache_command (&command_list, "setq mode=send");
      break;
      
    case 'H':
      util_cache_command (&command_list, "setq mode=headers");
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
      args->hint |= HINT_SEND_MODE;
      send_append_header2 (MU_HEADER_SUBJECT, arg, COMPOSE_REPLACE);
      util_cache_command (&command_list, "set noasksub");
      break;
      
    case 'u':
      args->user = arg;
      break;

    case 'E':
      util_cache_command (&command_list, "%s", arg);
      break;

    case F_ENCODING:
      free (default_encoding);
      default_encoding = mu_strdup (arg);
      break;
      
    case 'F':
      util_cache_command (&command_list, "set byname");
      break;

    case ARGP_KEY_ARG:
      args->argv = realloc (args->argv,
			    sizeof (char *) * (state->arg_num + 2));
      args->argv[state->arg_num] = arg;
      args->argv[state->arg_num + 1] = NULL;
      args->argc = state->arg_num + 1;
      break;

    case ARGP_KEY_FINI:
      if ((args->hint & (HINT_SEND_MODE|HINT_FILE_OPTION)) ==
	  (HINT_SEND_MODE|HINT_FILE_OPTION))
	argp_error (state, _("conflicting options"));
      else if (args->hint & HINT_FILE_OPTION)
	{
	  if (args->file)
	    {
	      if (args->argc > 1)
		argp_error (state,
			    _("-f requires at most one command line argument"));
	    }
	  else if (args->argc)
	    {
	      args->file = args->argv[0];
	  
	      if (args->argc > 1)
		argp_error (state,
			    _("-f requires at most one command line argument"));
	    }
	  else if (args->user)
	    mu_asprintf (&args->file, "~/%s/mbox", args->user);
	  else
	    args->file = "~/mbox";
	}
      else if (args->argc || (args->hint & HINT_SEND_MODE))
	util_cache_command (&command_list, "setq mode=send");
      else if (args->user)
	mu_asprintf (&args->file, "%%%s", args->user);
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
  "debug",
  "mailbox",
  "locking",
  NULL 
};
			     
static char *
mail_cmdline (void *closure, int cont MU_ARG_UNUSED)
{
  char *prompt = (char*) closure;
  char *rc;

  while (1)
    {
      if (mailvar_get (NULL, "autoinc", mailvar_type_boolean, 0) == 0
          && !mu_mailbox_is_updated (mbox))
	{
	  mu_mailbox_messages_count (mbox, &total);
	  page_invalidate (0);
	  mu_printf (_("New mail has arrived.\n"));
	}

      rc = ml_readline (prompt);
      
      if (ml_got_interrupt ())
	{
	  mu_error (_("Interrupt"));
	  continue;
	}

      if (!rc && mailvar_get (NULL, "ignoreeof", mailvar_type_boolean, 0) == 0)
	{
	  mu_error (_("Use \"quit\" to quit."));
	  continue;
	}

      break;
    }
  return rc;
}

static char *default_setup[] = {
  /* "set noallnet", */
  "setq append",
  "set asksub",
  "set crt",
  "set noaskbcc",
  "set askcc",
  "set noautoprint",
  "set nobang",
  "set nocmd",
  /*  "set nodebug",*/
  "set nodot",
  "set escape=~",
  "set noflipr",
  "set nofolder",
  "set header",
  "set nohold",
  "set noignore",
  "set noignoreeof",
  "set indentprefix=\"\t\"",
  "setq keep",
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
  "set fromfield",
  "set headline=\"%>%a%4m %18f %16d %3L/%-5o %s\"",
  
  /* Start in mail reading mode */
  "setq mode=read",
  "set noquit",
  "set rc",

  "set noflipr",
  "set noshowto",
  "set nobang",

  "set nullbody", /* Null message body is traditionally allowed */
  "set nullbodymsg=\"" N_("Null message body; hope that's ok") "\"",
  
  /* These settings are not yet used */
  "set noonehop",
  "set nosendwait",
};

static void
do_and_quit (const char *command)
{
  int rc = util_do_command ("%s", command);
  mu_mailbox_close (mbox);
  exit (rc != 0);
}

int
main (int argc, char **argv)
{
  char *mode = NULL, *prompt = NULL, *p;
  struct arguments args;
  int i, rc;
  
  mu_stdstream_setup (MU_STDSTREAM_RESET_NONE);
  set_cursor (1);

  /* Native Language Support */
  MU_APP_INIT_NLS ();
  
  /* Register the desired formats.  */
  mu_register_all_formats ();

  interactive = isatty (fileno (stdin));
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
  setenv ("DEAD", util_fullpath ("~/dead.letter"), 0);
  setenv ("EDITOR", "ed", 0);
  setenv ("LISTER", "ls", 0);
  setenv ("MAILRC", util_fullpath ("~/.mailrc"), 0);
  setenv ("MBOX", util_fullpath ("~/mbox"), 0);
  setenv ("PAGER", "more", 0);
  setenv ("SHELL", "sh", 0);
  setenv ("VISUAL", "vi", 0);
  setenv ("COLUMNS", "80", 0);
  setenv ("LINES", "24", 0);

  /* set defaults for execution */
  for (i = 0; i < sizeof (default_setup)/sizeof (default_setup[0]); i++)
    util_do_command ("%s", default_setup[i]);
  util_do_command ("set screen=%d", util_getlines ());
  util_do_command ("set columns=%d", util_getcols ());
  
  /* Set the default mailer to sendmail.  */
  mailvar_set ("sendmail",
	       mu_strdup ("sendmail:" PATH_SENDMAIL), mailvar_type_string,
	       MOPTF_OVERWRITE);

  args.argc = 0;
  args.argv = NULL;
  args.file = NULL;
  args.user = NULL;
  args.hint = 0;
  
  /* argument parsing */
#ifdef WITH_TLS
  mu_gocs_register ("tls", mu_tls_module_init);
#endif
  mu_argp_init (NULL, NULL);
  if (mu_app_init (&argp, mail_capa, NULL, argc, argv, 0, NULL, &args))
    exit (1);
  
  /* read system-wide mail.rc and user's .mailrc */
  if (mailvar_get (NULL, "rc", mailvar_type_boolean, 0) == 0)
    util_do_command ("source %s", SITE_MAIL_RC);
  if ((p = getenv ("MAILRC")) && *p)
    util_do_command ("source %s", getenv ("MAILRC"));

  util_run_cached_commands (&command_list);

  if (interactive)
    {
      /* Reset standard error stream so that it does not print program
	 name before the actual diagnostic message. */
      mu_stream_t errstr;
      int rc = mu_stdstream_strerr_create (&errstr, MU_STRERR_STDERR, 0, 0,
					   NULL, NULL);
      if (rc == 0)
	{
	  mu_stream_destroy (&mu_strerr);
	  mu_strerr = errstr;
	}
    }
  else
    {
      util_do_command ("set nocrt");
      util_do_command ("set noasksub");
      util_do_command ("set noaskcc");
      util_do_command ("set noaskbcc");
    }

  /* how should we be running? */
  if (mailvar_get (&mode, "mode", mailvar_type_string, 1))
    exit (EXIT_FAILURE);

  /* Interactive mode */

  ml_readline_init ();
  mail_set_my_name (args.user);

  /* Mode is just sending */
  if (strcmp (mode, "send") == 0)
    {
      char *buf = NULL;
      int rc;

      mu_argcv_string (args.argc, args.argv, &buf);
      rc = util_do_command ("mail %s", buf);
      return mailvar_get (NULL, "mailx", mailvar_type_boolean, 0) ? rc : 0;
    }
  /* Or acting as a normal reader */
  else 
    {
      if ((rc = mu_mailbox_create_default (&mbox, args.file)) != 0)
	{
	  if (args.file)
	    mu_error (_("Cannot create mailbox %s: %s"), args.file,
			mu_strerror (rc));
	  else
	    mu_error (_("Cannot create mailbox: %s"),
			mu_strerror (rc));
	  exit (EXIT_FAILURE);
	}

      if ((rc = mu_mailbox_open (mbox, MU_STREAM_RDWR|MU_STREAM_CREAT)) != 0)
	{
	  mu_url_t url = NULL;
	  mu_mailbox_get_url (mbox, &url);
	  mu_error (_("Cannot open mailbox %s: %s"),
		      mu_url_to_string (url), mu_strerror (rc));
	  mu_mailbox_destroy (&mbox);
	}

      if (rc)
	total = 0;
      else
	{
	  if ((rc = mu_mailbox_scan (mbox, 1, &total)) != 0)
	    {
	      mu_url_t url = NULL;
	      mu_mailbox_get_url (mbox, &url);
	      mu_error (_("Cannot read mailbox %s: %s"),
			  mu_url_to_string (url), mu_strerror (rc));
	      exit (EXIT_FAILURE);
	    }

	  if (strcmp (mode, "exist") == 0)
	    {
	      mu_mailbox_close (mbox);
	      return (total < 1) ? 1 : 0;
	    }
	  else if (strcmp (mode, "print") == 0)
	    do_and_quit ("print *");
	  else if (strcmp (mode, "headers") == 0)
	    do_and_quit ("from *");
	  else if (strcmp (mode, "read"))
	    {
	      mu_error (_("Unknown mode `%s'"), mode);
	      util_do_command ("quit");
	      return 1;
	    }
	}
      
      if (total == 0
	  && (strcmp (mode, "read")
	      || mailvar_get (NULL, "emptystart", mailvar_type_boolean, 0)))
        {
	  if (args.file)
	    mu_printf (_("%s: 0 messages\n"), args.file);
	  else
	    mu_printf (_("No mail for %s\n"),
			      args.user ? args.user : mail_whoami ());
          return 1;
        }

      /* initial commands */
      if (mailvar_get (NULL, "header", mailvar_type_boolean, 0) == 0)
	{
	  util_do_command ("summary");
	  util_do_command ("headers");
	}

      mailvar_get (&prompt, "prompt", mailvar_type_string, 0);
      mail_mainloop (mail_cmdline, (void*) prompt, 1);
      mu_printf ("\n");
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

  while ((command = (*input) (closure, 0)) != NULL)
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
	  buf = mu_alloc ((len + strlen (command2)) * sizeof (char));
	  strcpy (buf, command);
	  strcat (buf, command2);
	  free (command);
	  command = buf;
	  len = strlen (command);
	}
      cmd = mu_str_stripws (command);
      util_do_command ("%s", cmd);
#ifdef WITH_READLINE
      if (do_history && !(mu_isspace (cmd[0]) || cmd[0] == '#'))
	add_history (cmd);
#endif
      if (command)
	free (command);
    }
}

int
mail_warranty (int argc MU_ARG_UNUSED, char **argv MU_ARG_UNUSED)
{
  mu_printf (
	 _("GNU Mailutils -- a suite of utilities for electronic mail\n"
           "Copyright (C) 1999-2011 Free Software Foundation, Inc.\n\n"));
  mu_printf (
  _("   GNU Mailutils is free software; you can redistribute it and/or modify\n"
    "   it under the terms of the GNU General Public License as published by\n"
    "   the Free Software Foundation; either version 3 of the License, or\n"
    "   (at your option) any later version.\n"
    "\n"
    "   GNU Mailutils is distributed in the hope that it will be useful,\n"
    "   but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
    "   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
    "   GNU General Public License for more details.\n"
    "\n"
    "   You should have received a copy of the GNU General Public License along\n"
    "   with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>.\n" 
    "\n"
    "\n"
));

  return 0;
}

