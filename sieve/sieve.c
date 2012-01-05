/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2003, 2005-2012 Free Software Foundation, Inc.

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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include <sysexits.h>

#include <mailutils/io.h>
#include <mailutils/argcv.h>
#include <mailutils/sieve.h>
#include <mailutils/auth.h>
#include <mailutils/errno.h>
#include <mailutils/folder.h>
#include <mailutils/list.h>
#include <mailutils/mailbox.h>
#include <mailutils/util.h>
#include <mailutils/registrar.h>
#include <mailutils/stream.h>
#include <mailutils/nls.h>
#include <mailutils/tls.h>

#include "mailutils/libargp.h"

static char doc[] =
N_("GNU sieve -- a mail filtering tool.")
"\v"
N_("Debug flags:\n\
  g - main parser traces\n\
  T - mailutils traces (same as --debug-level=sieve.trace0-trace1)\n\
  P - network protocols (same as --debug-level=sieve.=prot)\n\
  t - sieve trace (MU_SIEVE_DEBUG_TRACE)\n\
  i - sieve instructions trace (MU_SIEVE_DEBUG_INSTR)\n");

#define D_DEFAULT "TPt"

#define ARG_LINE_INFO 257
#define ARG_NO_PROGRAM_NAME 258

static struct argp_option options[] =
{
  {"no-actions", 'n', 0, 0,
   N_("do not execute any actions, just print what would be done"), 0},
  {"dry-run", 0, NULL, OPTION_ALIAS, NULL },
  {"keep-going", 'k', 0, 0,
   N_("keep on going if execution fails on a message"), 0},

  {"compile-only", 'c', 0, 0,
   N_("compile script and exit"), 0},

  {"dump", 'D', 0, 0,
   N_("compile script, dump disassembled sieve code to terminal and exit"), 0 },
  
  {"mbox-url", 'f', N_("MBOX"), 0,
   N_("mailbox to sieve (defaults to user's mail spool)"), 0},

  {"ticket", 't', N_("TICKET"), 0,
   N_("ticket file for user authentication"), 0},

  {"debug", 'd', N_("FLAGS"), OPTION_ARG_OPTIONAL,
   N_("debug flags (defaults to \"" D_DEFAULT "\")"), 0},

  {"verbose", 'v', NULL, 0,
   N_("log all actions"), 0},

  {"line-info", ARG_LINE_INFO, N_("BOOL"), OPTION_ARG_OPTIONAL,
   N_("print source location along with action logs (default)") },
   
  {"email", 'e', N_("ADDRESS"), 0,
   N_("override user email address"), 0},
  {"expression", 'E', NULL, 0,
   N_("treat SCRIPT as Sieve program text"), 0},
  
  {"no-program-name", ARG_NO_PROGRAM_NAME, NULL, 0,
   N_("do not prefix diagnostic messages with the program name"), 0},
  
  {0}
};

int keep_going;
int compile_only;
char *mbox_url;
int sieve_debug;
int verbose;
char *script;
int expression_option;

static int sieve_print_locus = 1; /* Should the log messages include the
				     locus */

static int
is_true_p (char *p)
{
  int rc;
  if (!p)
    return 1;
  if ((rc = mu_true_answer_p (p)) == -1)
    return 0;
  return rc;
}

static void
set_debug_level (const char *arg)
{
  mu_debug_level_t lev;
  
  for (; *arg; arg++)
    {
      switch (*arg)
	{
	case 'T':
	  mu_debug_get_category_level (mu_sieve_debug_handle, &lev);
	  mu_debug_set_category_level (mu_sieve_debug_handle, 
				       lev |
				    (MU_DEBUG_LEVEL_UPTO(MU_DEBUG_TRACE9) &
				     ~MU_DEBUG_LEVEL_MASK(MU_DEBUG_ERROR)));
	  break;

	case 'P':
	  mu_debug_get_category_level (mu_sieve_debug_handle, &lev);
	  mu_debug_set_category_level (mu_sieve_debug_handle,
				    lev | MU_DEBUG_LEVEL_MASK(MU_DEBUG_PROT));
	  break;

	case 'g':
	  mu_sieve_yydebug = 1;
	  break;

	case 't':
	  sieve_debug |= MU_SIEVE_DEBUG_TRACE;
	  break;
	  
	case 'i':
	  sieve_debug |= MU_SIEVE_DEBUG_INSTR;
	  break;
	  
	default:
	  mu_error (_("%c is not a valid debug flag"), *arg);
	}
    }
}

int
mu_compat_printer (void *data, mu_log_level_t level, const char *buf)
{
  fputs (buf, stderr);
  return 0;
}

static error_t
parser (int key, char *arg, struct argp_state *state)
{
  static mu_list_t lst;
  
  switch (key)
    {
    case 'E':
      expression_option = 1;
      break;
      
    case 'e':
      mu_argp_node_list_new (lst, "email", arg);
      break;
      
    case 'n':
      sieve_debug |= MU_SIEVE_DRY_RUN;
      mu_argp_node_list_new (lst, "verbose", "yes");
      break;

    case 'k':
      mu_argp_node_list_new (lst, "keep-going", "yes");
      break;

    case 'c':
      compile_only = 1;
      break;

    case 'D':
      compile_only = 2;
      break;
      
    case 'f':
      mu_argp_node_list_new (lst, "mbox-url", arg);
      break;
      
    case 't':
      mu_argp_node_list_new (lst, "ticket", arg);
      break;
      
    case 'd':
      mu_argp_node_list_new (lst, "debug", arg ? arg : D_DEFAULT);
      break;

    case 'v':
      mu_argp_node_list_new (lst, "verbose", "yes");
      break;

    case ARG_LINE_INFO:
      mu_argp_node_list_new (lst, "line-info",
			     is_true_p (arg) ? "yes" : "no");
      break;

    case ARG_NO_PROGRAM_NAME:
      mu_log_tag = NULL;
      break;
      
    case ARGP_KEY_ARG:
      if (script)
	argp_error (state, _("Only one SCRIPT can be specified"));
      script = mu_tilde_expansion (arg, MU_HIERARCHY_DELIMITER, NULL);
      break;

    case ARGP_KEY_INIT:
      mu_argp_node_list_init (&lst);
      break;
      
    case ARGP_KEY_FINI:
      mu_argp_node_list_finish (lst, NULL, NULL);
      break;
      
    default:
      return ARGP_ERR_UNKNOWN;
    }

  return 0;
}

static struct argp argp =
{
  options,
  parser,
  N_("SCRIPT"),
  doc
};


static int 
cb_debug (void *data, mu_config_value_t *val)
{
  if (mu_cfg_assert_value_type (val, MU_CFG_STRING))
    return 1;
  set_debug_level (val->v.string);
  return 0;
}

static int
cb_email (void *data, mu_config_value_t *val)
{
  int rc;
  
  if (mu_cfg_assert_value_type (val, MU_CFG_STRING))
    return 1;
  rc = mu_set_user_email (val->v.string);
  if (rc)
    mu_error (_("invalid email: %s"), mu_strerror (rc));
  return rc;
}

static struct mu_cfg_param sieve_cfg_param[] = {
  { "keep-going", mu_cfg_bool, &keep_going, 0, NULL,
    N_("Do not abort if execution fails on a message.") },
  { "mbox-url", mu_cfg_string, &mbox_url, 0, NULL,
    N_("Mailbox to sieve (defaults to user's mail spool)."),
    N_("url") },
  { "ticket", mu_cfg_string, &mu_ticket_file, 0, NULL,
    N_("Ticket file for user authentication."),
    N_("ticket") },
  { "debug", mu_cfg_callback, NULL, 0, cb_debug,
    N_("Debug flags.  Argument consists of one or more of the following "
       "flags:\n"
       "   g - main parser traces\n"
       "   T - mailutils traces (sieve.trace9)\n"
       "   P - network protocols (sieve.prot)\n"
       "   t - sieve trace (MU_SIEVE_DEBUG_TRACE)\n"
       "   i - sieve instructions trace (MU_SIEVE_DEBUG_INSTR).") },
  { "verbose", mu_cfg_bool, &verbose, 0, NULL,
    N_("Log all executed actions.") },
  { "line-info", mu_cfg_bool, &sieve_print_locus, 0, NULL,
    N_("Print source locations along with action logs (default).") },
  { "email", mu_cfg_callback, NULL, 0, cb_email,
    N_("Set user email address.") },
  { NULL }
};


static const char *sieve_argp_capa[] =
{
  "common",
  "debug",
  "mailbox",
  "locking",
  "logging",
  "mailer",
  NULL
};

static void
_sieve_action_log (void *unused,
		   mu_stream_t stream, size_t msgno,
		   mu_message_t msg,
		   const char *action, const char *fmt, va_list ap)
{
  size_t uid = 0;
  
  mu_message_get_uid (msg, &uid);
  mu_stream_printf (stream, "\033s<%d>\033%c<%d>", MU_LOG_NOTICE,
		    sieve_print_locus ? 'O' : 'X', MU_LOGMODE_LOCUS);
  mu_stream_printf (stream, _("%s on msg uid %lu"),
		  action, (unsigned long) uid);
  
  if (fmt && strlen (fmt))
    {
      mu_stream_printf (stream, ": ");
      mu_stream_vprintf (stream, fmt, ap);
    }
  mu_stream_printf (stream, "\n");
}

static int
sieve_message (mu_sieve_machine_t mach)
{
  int rc;
  mu_stream_t instr;
  mu_message_t msg;
  mu_attribute_t attr;

  rc = mu_stdio_stream_create (&instr, MU_STDIN_FD, MU_STREAM_SEEK);
  if (rc)
    {
      mu_error (_("cannot create stream: %s"), mu_strerror (rc));
      return EX_SOFTWARE;
    }
  rc = mu_stream_to_message (instr, &msg);
  mu_stream_unref (instr);
  if (rc)
    {
      mu_error (_("cannot create message from stream: %s"),
		mu_strerror (rc));
      return EX_SOFTWARE;
    }
  mu_message_get_attribute (msg, &attr);
  mu_attribute_unset_deleted (attr);
  rc = mu_sieve_message (mach, msg);
  if (rc)
    /* FIXME: switch (rc)...*/
    return EX_SOFTWARE;

  return mu_attribute_is_deleted (attr) ? 1 : EX_OK;
}

static int
sieve_mailbox (mu_sieve_machine_t mach)
{
  int rc;
  mu_mailbox_t mbox = NULL;
  
  /* Create and open the mailbox. */
  if ((rc = mu_mailbox_create_default (&mbox, mbox_url)) != 0)
    {
      if (mbox)
	mu_error (_("could not create mailbox `%s': %s"),
		  mbox_url, mu_strerror (rc));
      else
	mu_error (_("could not create default mailbox: %s"),
		  mu_strerror (rc));
      goto cleanup;
    }

  /* Open the mailbox read-only if we aren't going to modify it. */
  if (sieve_debug & MU_SIEVE_DRY_RUN)
    rc = mu_mailbox_open (mbox, MU_STREAM_READ);
  else
    rc = mu_mailbox_open (mbox, MU_STREAM_RDWR);
  
  if (rc != 0)
    {
      if (mbox)
	{
	  mu_url_t url = NULL;

	  mu_mailbox_get_url (mbox, &url);
	  mu_error (_("cannot open mailbox %s: %s"),
		    mu_url_to_string (url), mu_strerror (rc));
	}
      else
	mu_error (_("cannot open default mailbox: %s"),
		  mu_strerror (rc));
      mu_mailbox_destroy (&mbox);
      goto cleanup;
    }
  
  /* Process the mailbox */
  rc = mu_sieve_mailbox (mach, mbox);
  
 cleanup:
  if (mbox && !(sieve_debug & MU_SIEVE_DRY_RUN))
    {
      int e;
      
      /* A message won't be marked deleted unless the script executed
         succesfully on it, so we always do an expunge, it will delete
         any messages that were marked DELETED even if execution failed
         on a later message. */
      if ((e = mu_mailbox_expunge (mbox)) != 0)
	{
	  if (mbox)
	    mu_error (_("expunge on mailbox `%s' failed: %s"),
		      mbox_url, mu_strerror (e));
	  else
	    mu_error (_("expunge on default mailbox failed: %s"),
		      mu_strerror (e));
	}
      
      if (e && !rc)
	rc = e;
    }
  
  mu_sieve_machine_destroy (&mach);
  mu_mailbox_close (mbox);
  mu_mailbox_destroy (&mbox);
  /* FIXME: switch (rc) ... */
  return rc ? EX_SOFTWARE : EX_OK;
}

int
main (int argc, char *argv[])
{
  mu_sieve_machine_t mach;
  int rc;

  /* Native Language Support */
  MU_APP_INIT_NLS ();

  mu_argp_init (NULL, NULL);
#ifdef WITH_TLS
  mu_gocs_register ("tls", mu_tls_module_init);
#endif
  mu_gocs_register ("sieve", mu_sieve_module_init);
  mu_sieve_debug_init ();
  
  mu_register_all_formats ();

  if (mu_app_init (&argp, sieve_argp_capa, sieve_cfg_param, 
		   argc, argv, ARGP_IN_ORDER, NULL, NULL))
    exit (EX_USAGE);
  
  if (!script)
    {
      mu_error (_("script must be specified"));
      exit (EX_USAGE);
    }
  
  /* Sieve interpreter setup. */
  rc = mu_sieve_machine_init (&mach);
  if (rc)
    {
      mu_error (_("cannot initialize sieve machine: %s"), mu_strerror (rc));
      return EX_SOFTWARE;
    }

  if (verbose)
    mu_sieve_set_logger (mach, _sieve_action_log);

  if (expression_option)
    rc = mu_sieve_compile_buffer (mach, script, strlen (script),
				  "stdin", 1);
  else
    rc = mu_sieve_compile (mach, script);
  if (rc)
    return EX_CONFIG;

  /* We can finish if it's only a compilation check. */
  if (compile_only)
    {
      if (compile_only == 2)
	mu_sieve_disass (mach);
      return EX_OK;
    }

  mu_sieve_set_debug_level (mach, sieve_debug);

  if (mbox_url && strcmp (mbox_url, "-") == 0)
    rc = sieve_message (mach);
  else
    rc = sieve_mailbox (mach);

  return rc;
}

