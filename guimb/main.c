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

#include "guimb.h"
#include "getopt.h"

static char short_options[] = "c:de:f:g:hM:m:s:u::v{";
static struct option long_options[] = {
  {"code", required_argument, 0, 'c'},
  {"debug", no_argument, 0, 'd'},
  {"expression", required_argument, 0, 'e'},
  {"file", required_argument, 0, 'f'},
  {"help", no_argument, 0, 'h'},
  {"guile-arg", required_argument, 0, 'g'},
  {"maildir", required_argument, 0, 'm'},
  {"mailbox", required_argument, 0, 'M'},
  {"source", required_argument, 0, 's'},
  {"user", optional_argument, 0, 'u'},
  {"version", no_argument, 0, 'v'},
  {0, 0, 0, 0}
};

char *program_file;
char *program_expr;
int debug_guile;
char *user_name;
char *default_mailbox;
char *maildir = MU_PATH_MAILDIR;

static void usage (void);
char * who_am_i ();

static int g_size;
static int g_argc;
static char **g_argv;

#define ARG_INC 16

void
append_arg (char *arg)
{
  if (g_argc == g_size)
    {
      g_size += ARG_INC;
      g_argv = realloc (g_argv, g_size * sizeof (g_argv[0]));
      if (!g_argv)
	{
	  util_error ("not enough memory");
	  exit (1);
	}
    }
  g_argv[g_argc++] = arg;
}

int
main (int argc, char *argv[])
{
  int c;
  int stop = 0;
  guimb_param_t param;
  struct guimb_data gd;
  
  append_arg ("");
  while (!stop
	 && (c = getopt_long (argc, argv, short_options, long_options, NULL))
	     != -1)
    switch (c)
      {
      case 'c':
	program_expr = optarg;
	stop = 1;
	break;
      case 'd':
	debug_guile = 1;
	break;
      case 'f':
	program_file = optarg;
	break;
      case 'e':
	program_expr = optarg;
	break;
      case 'g':
	append_arg (optarg);
	break;
      case 'h':
	usage ();
	exit (0);
      case 'M':
	default_mailbox = optarg;
	break;
      case 'm':
	maildir = optarg;
	break;
      case 'u':
	user_name = optarg ? optarg : who_am_i ();
	break;
      case 's':
	program_file = optarg;
	stop = 1;
	break;
      case 'v':
	printf ("guimb (" PACKAGE " " VERSION ")\n");
	exit (0);
      case '{':
	for (;; optind++)
	  {
	    if (optind == argc)
	      {
		fprintf (stderr, "guimb: missing -}\n");
		exit (1);
	      }
	    if (strcmp (argv[optind], "-}") == 0)
	      break;
	    append_arg (argv[optind]);
	  }
	optind++;
	break;
      default:
	exit (1);
      }

  if (stop)
    for (; optind < argc; optind++)
      append_arg (argv[optind]);

  if (!user_name)
    user_name = who_am_i ();
  
  maildir = mu_normalize_maildir (maildir);
  if (!maildir)
    {
      util_error ("Badly formed maildir: %s", maildir);
      exit (1);
    }
  
  if (program_file)
    g_argv[0] = program_file;
  else if (!program_expr)
    {
      usage ();
      exit (0);
    }
    
  /* Register the desired formats. */
  {
    list_t lst;
    registrar_get_list (&lst);
    list_append (lst, mbox_record);
    list_append (lst, path_record);
    list_append (lst, pop_record);
    list_append (lst, imap_record);
    /* Possible supported mailers.  */
    list_append (lst, sendmail_record);
    list_append (lst, smtp_record);
  }

  if (!argv[optind])
    {
      if (default_mailbox)
	append_arg (default_mailbox);
      collect_open_default ();
    }
  else
    {
      collect_open_mailbox_file ();

      if (argv[optind])
	{
	  for (; argv[optind]; optind++)
	    {
	      append_arg (argv[optind]);
	      collect_append_file (argv[optind]);
	    }
	}
      else 
	collect_append_file ("-");
    }

  append_arg (NULL);
  g_argc--;

  /* Finish creating input mailbox */
  collect_create_mailbox ();

  gd.program_file = program_file;
  gd.program_expr = program_expr;
  
  param.debug_guile = debug_guile;
  param.mbox = mbox;
  param.user_name = user_name;
  param.init = NULL;
  param.catch_body = guimb_catch_body;
  param.catch_handler = guimb_catch_handler;
  param.next = NULL;
  param.exit = guimb_exit;
  param.data = &gd;
  mu_process_mailbox (g_argc, g_argv, &param);
  /*NOTREACHED*/
  return 0;
}

char *
who_am_i ()
{
  char *name = getenv ("LOGNAME");
  if (!name)
    {
      name = getenv ("USER");
      if (!name)
	name = strdup (getlogin ());
    }
  return name;
}

static char usage_str[] =
  "Usage: guimb [OPTIONS] [-{ SCRIPT-OPTIONS -}] [MBOX ...]\n"
  "Process the contents of the specified mailboxes using a Scheme program\n"
  "or expression.\n"
  "Options are:\n\n"
  "  -c, --code EXPR           Execute given scheme expression.\n"
  "  -s, --source PROGFILE     Load Scheme source code from PROGFILE, and exit\n"
  "\nThe above switches stop argument processing, and pass all\n"
  "remaining arguments as the value of (command-line).\n"
  "\n"
  "  -d, --debug               Start with debugging evaluator and backtraces.\n"
  "  -e, --expression EXPR     Execute scheme expression.\n"
  "  -f, --file PROGFILE       Read program from PROGFILE.\n"
  "  -g, --guile-arg ARG       Append ARG to the command line passed to scheme\n"
  "                            program.\n"
  "  -m, --mailbox MBOX        Set default mailbox name.\n"
  "  -u, --user NAME           Act as local MDA for user NAME.\n"
  "  -h, --help                Display help message.\n"
  "  -v, --version             Display program version.\n"
  "\n"
  "Any arguments between -{ and -} are passed to the Scheme program verbatim.\n"
  "When both --file and --expression are specified, file is evaluated first.\n"
  "If no mailboxes are specified, the system mailbox of the current user is read.\n\n"
  "The semantics of the default mailbox depends on whether more mailbox\n"
  "arguments are specified in the command line. If they are, any messages\n"
  "that are not deleted after executing the script are appended to the default\n"
  "mailbox. Otherwise its contents is read, processed and *replaced* by\n" 
  "messages that remain undeleted after executing the script.\n";

void
usage ()
{
  printf ("%s\n" "Report bugs to <bug-mailutils@gnu.org>\n", usage_str);
}
