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

#include <mailutils/argp.h>

char *program_file;
char *program_expr;
int debug_guile;
char *user_name;
char *default_mailbox;

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

static struct argp_option options[] = {
  {NULL, 0, NULL, 0,
   "The following switches stop argument processing, and pass all\n"
   "remaining arguments as the value of (command-line):", 1},
  {"code", 'c', "EXPR", 0, "Execute given scheme expression.", 1},
  {"source", 's', "PROGFILE", 0,
   "Load Scheme source code from PROGFILE, and exit", 1},
  {NULL, 0, NULL, 0,
   "The following options do not change the order of options parsing:", 2},
  {"expression", 'e', "EXPR", 0, "Execute given scheme expression.", 2},
  {"file", 'f', "PROGFILE", 0,
   "Load Scheme source code from PROGFILE, and exit", 2},
  {NULL, 0, NULL, 0, "Other options:", 3},
  {"debug", 'd', NULL, 0, "Start with debugging evaluator and backtraces.", 3},
  {"guile-arg", 'g', "ARG", 0,
   "Append ARG to the command line passed to Guile", 3},
  {"mailbox", 'M', "NAME", 0, "Set default mailbox name", 3},
  {"user", 'u', "NAME", OPTION_ARG_OPTIONAL,
   "Act as local MDA for user NAME", 3},
  {0, 0, 0, 0}
};

static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  switch (key)
    {
    case 'c':
      program_expr = optarg;
      *(int *)state->input = state->next;
      state->next = state->argc;
      break;
      
    case 's':
      program_file = optarg;
      *(int *)state->input = state->next;
      state->next = state->argc;
      break;

    case 'f':
      program_file = optarg;
      break;

    case 'e':
      program_expr = optarg;
      break;

    case 'd':
      debug_guile = 1;
      break;

    case 'g':
      append_arg (optarg);
      break;

    case 'M':
      default_mailbox = optarg;
      break;

    case 'u':
      user_name = optarg ? optarg : who_am_i ();
      break;

    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

const char *argp_program_version = "guimb (" PACKAGE ") " VERSION;
const char *argp_program_bug_address = "<bug-mailutils@gnu.org>";
static char doc[] =
"GNU messages -- Process the contents of the specified mailboxes\n"
"using a Scheme program or expression.";
static char args_doc[] = "[mailbox...]";

static struct argp argp = {
  options,
  parse_opt,
  args_doc,
  doc,
  NULL,
  NULL, NULL
};

static const char *guimb_argp_capa[] = {
  "common",
  "mailbox",
  "license",
  NULL
};
    
int
main (int argc, char *argv[])
{
  int c = argc;
  guimb_param_t param;
  struct guimb_data gd;
  
  append_arg ("");
  mu_argp_parse (&argp, &argc, &argv, 0, guimb_argp_capa, NULL, &c);
  
  for (; c < argc; c++)
      append_arg (argv[c]);

  if (!user_name)
    user_name = who_am_i ();
  
  if (program_file)
    g_argv[0] = program_file;
  else if (!program_expr)
    {
      mu_error ("At least one of -fecs must be used. Try guimb --help for more info");
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

