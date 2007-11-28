/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002, 2003, 
   2005, 2006, 2007 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
   MA 02110-1301 USA */

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

#include <mu_asprintf.h>
#include <mailutils/argcv.h>
#include <mailutils/libsieve.h>
#include <mailutils/auth.h>
#include <mailutils/errno.h>
#include <mailutils/folder.h>
#include <mailutils/list.h>
#include <mailutils/mailbox.h>
#include <mailutils/mutil.h>
#include <mailutils/registrar.h>
#include <mailutils/stream.h>
#include <mailutils/nls.h>
#include <mailutils/tls.h>

#include "mailutils/libargp.h"

const char *program_version = "sieve (" PACKAGE_STRING ")";

static char doc[] =
N_("GNU sieve -- a mail filtering tool")
"\v"
N_("Debug flags:\n\
  g - main parser traces\n\
  T - mailutils traces (MU_DEBUG_TRACE0-MU_DEBUG_TRACE1)\n\
  P - network protocols (MU_DEBUG_PROT)\n\
  t - sieve trace (MU_SIEVE_DEBUG_TRACE)\n\
  i - sieve instructions trace (MU_SIEVE_DEBUG_INSTR)\n");

#define D_DEFAULT "TPt"

#define ARG_LINE_INFO 257

static struct argp_option options[] =
{
  {"no-actions", 'n', 0, 0,
   N_("Do not execute any actions, just print what would be done"), 0},
  {"dry-run", 0, NULL, OPTION_ALIAS, NULL },
  {"keep-going", 'k', 0, 0,
   N_("Keep on going if execution fails on a message"), 0},

  {"compile-only", 'c', 0, 0,
   N_("Compile script and exit"), 0},

  {"dump", 'D', 0, 0,
   N_("Compile script, dump disassembled sieve code to terminal and exit"), 0 },
  
  {"mbox-url", 'f', N_("MBOX"), 0,
   N_("Mailbox to sieve (defaults to user's mail spool)"), 0},

  {"ticket", 't', N_("TICKET"), 0,
   N_("Ticket file for mailbox authentication"), 0},

  {"debug", 'd', N_("FLAGS"), OPTION_ARG_OPTIONAL,
   N_("Debug flags (defaults to \"" D_DEFAULT "\")"), 0},

  {"verbose", 'v', NULL, 0,
   N_("Log all actions"), 0},

  {"line-info", ARG_LINE_INFO, N_("BOOL"), OPTION_ARG_OPTIONAL,
   N_("Print source location along with action logs (default)") },
   
  {"email", 'e', N_("ADDRESS"), 0,
   N_("Override user email address"), 0},
  
  {0}
};

int keep_going;
int compile_only;
char *mbox_url;
char *tickets;
int tickets_default;
int debug_level;
int sieve_debug;
int verbose;
char *script;

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
set_debug_level (mu_debug_t debug, const char *arg)
{
  for (; *arg; arg++)
    {
      switch (*arg)
	{
	case 'T':
	  debug_level |= MU_DEBUG_LEVEL_UPTO (MU_DEBUG_TRACE7);
	  break;

	case 'P':
	  debug_level |= MU_DEBUG_LEVEL_MASK (MU_DEBUG_PROT);
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
	  if (debug)
	    mu_cfg_format_error (debug, MU_DEBUG_ERROR,
				 _("%c is not a valid debug flag"), *arg);
	  else
	    mu_error (_("%c is not a valid debug flag"), *arg);
	}
    }
}

static error_t
parser (int key, char *arg, struct argp_state *state)
{
  int rc;
  
  switch (key)
    {
    case 'e':
      rc = mu_set_user_email (arg);
      if (rc)
	argp_error (state, _("Invalid email: %s"), mu_strerror (rc));
      break;
      
    case 'n':
      sieve_debug |= MU_SIEVE_DRY_RUN;
      break;

    case 'k':
      keep_going = 1;
      break;

    case 'c':
      compile_only = 1;
      break;

    case 'D':
      compile_only = 2;
      break;
      
    case 'f':
      if (mbox_url)
	free (mbox_url);
      mbox_url = strdup (arg);
      break;
      
    case 't':
      free (tickets);
      tickets = mu_tilde_expansion (arg, "/", NULL);
      tickets_default = 0;
      break;
      
    case 'd':
      if (!arg)
	arg = D_DEFAULT;
      set_debug_level (NULL, arg);
      break;

    case 'v':
      verbose = 1;
      break;

    case ARG_LINE_INFO:
      sieve_print_locus = is_true_p (arg);
      break;
      
    case ARGP_KEY_ARG:
      if (script)
	argp_error (state, _("Only one SCRIPT can be specified"));
      script = mu_tilde_expansion (arg, "/", NULL);
      break;

    case ARGP_KEY_NO_ARGS:
      argp_error (state, _("SCRIPT must be specified"));

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
cb_debug (mu_debug_t debug, void *data, char *arg)
{
  set_debug_level (debug, arg);
  return 0;
}

static int
cb_email (mu_debug_t debug, void *data, char *arg)
{
  int rc = mu_set_user_email (arg);
  if (rc)
    mu_cfg_format_error (debug, MU_DEBUG_ERROR, _("Invalid email: %s"),
			 mu_strerror (rc));
  return rc;
}

static int
cb_ticket (mu_debug_t debug, void *data, char *arg)
{
  free (tickets);
  tickets = mu_tilde_expansion (arg, "/", NULL);
  tickets_default = 0;
  return 0;
}

static struct mu_cfg_param sieve_cfg_param[] = {
  { "keep-going", mu_cfg_int, &keep_going },
  { "mbox-url", mu_cfg_string, &mbox_url },
  { "ticket", mu_cfg_callback, NULL, cb_ticket },
  { "debug", mu_cfg_callback, NULL, cb_debug },
  { "verbose", mu_cfg_bool, &verbose },
  { "line-info", mu_cfg_bool, &sieve_print_locus },
  { "email", mu_cfg_callback, NULL, cb_email },
  { NULL }
};


static const char *sieve_argp_capa[] =
{
  "common",
  "debug",
  "mailbox",
  "locking",
  "license",
  "logging",
  "mailer",
  NULL
};

static int
sieve_stderr_debug_printer (void *unused, const char *fmt, va_list ap)
{
  vfprintf (stderr, fmt, ap);
  return 0;
}

static int
sieve_syslog_debug_printer (void *unused, const char *fmt, va_list ap)
{
  vsyslog (LOG_DEBUG, fmt, ap);
  return 0;
}

static void
stdout_action_log (void *unused,
		   const mu_sieve_locus_t *locus, size_t msgno, mu_message_t msg,
		   const char *action, const char *fmt, va_list ap)
{
  size_t uid = 0;
  
  mu_message_get_uid (msg, &uid);

  if (sieve_print_locus)
    fprintf (stdout, _("%s:%lu: %s on msg uid %lu"),
	     locus->source_file, (unsigned long) locus->source_line,
	     action, (unsigned long) uid);
  else
    fprintf (stdout, _("%s on msg uid %lu"), action, (unsigned long) uid);
  
  if (fmt && strlen (fmt))
    {
      fprintf (stdout, ": ");
      vfprintf (stdout, fmt, ap);
    }
  fprintf (stdout, "\n");
}

static void
syslog_action_log (void *unused,
		   const mu_sieve_locus_t *locus, size_t msgno, mu_message_t msg,
		   const char *action, const char *fmt, va_list ap)
{
  size_t uid = 0;
  char *text = NULL;
  
  mu_message_get_uid (msg, &uid);

  if (sieve_print_locus)
    asprintf (&text, _("%s:%lu: %s on msg uid %lu"),
	      locus->source_file, (unsigned long) locus->source_line,
	      action, (unsigned long) uid);
  else
    asprintf (&text, _("%s on msg uid %lu"), action, (unsigned long) uid);
    
  if (fmt && strlen (fmt))
    {
      char *diag = NULL;
      asprintf (&diag, fmt, ap);
      syslog (LOG_NOTICE, "%s: %s", text, diag);
      free (diag);
    }
  else
    syslog (LOG_NOTICE, "%s", text);
  free (text);
}

int
main (int argc, char *argv[])
{
  mu_sieve_machine_t mach;
  mu_wicket_t wicket = 0;
  mu_ticket_t ticket = 0;
  mu_debug_t debug = 0;
  mu_mailbox_t mbox = 0;
  int rc;

  /* Native Language Support */
  mu_init_nls ();

  mu_argp_init (program_version, NULL);
#ifdef WITH_TLS
  mu_gocs_register ("tls", mu_tls_module_init);
#endif
  mu_gocs_register ("sieve", mu_sieve_module_init);

  mu_register_all_formats ();

  tickets = mu_tilde_expansion ("~/.tickets", "/", NULL);
  tickets_default = 1;
  debug_level = MU_DEBUG_ERROR;
  log_facility = 0;

  if (mu_app_init (&argp, sieve_argp_capa, sieve_cfg_param, 
		   argc, argv, ARGP_IN_ORDER, NULL, NULL))
    exit (1);

  /* Sieve interpreter setup. */
  rc = mu_sieve_machine_init (&mach, NULL);
  if (rc)
    {
      mu_error (_("Cannot initialize sieve machine: %s"), mu_strerror (rc));
      return 1;
    }

  if (log_facility)
    {
      openlog ("sieve", LOG_PID, log_facility);
      mu_error_set_print (mu_syslog_error_printer);
      mu_sieve_set_debug (mach, sieve_syslog_debug_printer);
      if (verbose)
	mu_sieve_set_logger (mach, syslog_action_log);
    }
  else
    {
      mu_sieve_set_debug (mach, sieve_stderr_debug_printer);
      if (verbose)
	mu_sieve_set_logger (mach, stdout_action_log);
    }
  
  rc = mu_sieve_compile (mach, script);
  if (rc)
    return 1;

  /* We can finish if its only a compilation check. */
  if (compile_only)
    {
      if (compile_only == 2)
	mu_sieve_disass (mach);
      return 0;
    }

  /* Create a ticket, if we can. */
  if (tickets)
    {
      if ((rc = mu_wicket_create (&wicket, tickets)) == 0)
        {
          if ((rc = mu_wicket_get_ticket (wicket, &ticket, 0, 0)) != 0)
            {
              mu_error (_("ticket_get failed: %s"), mu_strerror (rc));
              goto cleanup;
            }
        }
      else if (!(tickets_default && errno == ENOENT))
        {
          mu_error (_("mu_wicket_create `%s' failed: %s"),
                    tickets, mu_strerror (rc));
          goto cleanup;
        }
      if (ticket)
        mu_sieve_set_ticket (mach, ticket);
    }

  /* Create a debug object, if needed. */
  if (debug_level)
    {
      if ((rc = mu_debug_create (&debug, mach)))
	{
	  mu_error (_("mu_debug_create failed: %s"), mu_strerror (rc));
	  goto cleanup;
	}
      if ((rc = mu_debug_set_level (debug, debug_level)))
	{
	  mu_error (_("mu_debug_set_level failed: %s"),
		    mu_strerror (rc));
	  goto cleanup;
	}
      mu_sieve_set_debug_object (mach, debug);
    }
  
  mu_sieve_set_debug_level (mach, sieve_debug);
    
  /* Create, give a ticket to, and open the mailbox. */
  if ((rc = mu_mailbox_create_default (&mbox, mbox_url)) != 0)
    {
      if (mbox)
	mu_error (_("Could not create mailbox `%s': %s"),
		  mbox_url, mu_strerror (rc));
      else
	mu_error (_("Could not create default mailbox: %s"),
		  mu_strerror (rc));
      goto cleanup;
    }

  if (debug && (rc = mu_mailbox_set_debug (mbox, debug)))
    {
      mu_error (_("mu_mailbox_set_debug failed: %s"), mu_strerror (rc));
      goto cleanup;
    }

  if (ticket)
    {
      mu_folder_t folder = NULL;
      mu_authority_t auth = NULL;

      if ((rc = mu_mailbox_get_folder (mbox, &folder)))
	{
	  mu_error (_("mu_mailbox_get_folder failed: %s"),
		   mu_strerror (rc));
	  goto cleanup;
	}

      if ((rc = mu_folder_get_authority (folder, &auth)))
	{
	  mu_error (_("mu_folder_get_authority failed: %s"),
		   mu_strerror (rc));
	  goto cleanup;
	}

      /* Authentication-less folders don't have authorities. */
      if (auth && (rc = mu_authority_set_ticket (auth, ticket)))
	{
	  mu_error (_("mu_authority_set_ticket failed: %s"),
		   mu_strerror (rc));
	  goto cleanup;
	}
    }

  /* Open the mailbox read-only if we aren't going to modify it. */
  if (sieve_debug & MU_SIEVE_DRY_RUN)
    rc = mu_mailbox_open (mbox, MU_STREAM_READ);
  else
    rc = mu_mailbox_open (mbox, MU_STREAM_RDWR);

  if (rc != 0)
    {
      if (mbox)
	mu_error (_("Opening mailbox `%s' failed: %s"),
		  mbox_url, mu_strerror (rc));
      else
	mu_error (_("Opening default mailbox failed: %s"),
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
	    mu_error (_("Expunge on mailbox `%s' failed: %s"),
		      mbox_url, mu_strerror (e));
	  else
	    mu_error (_("Expunge on default mailbox failed: %s"),
		      mu_strerror (e));
	}
      
      if (e && !rc)
	rc = e;
    }

  mu_sieve_machine_destroy (&mach);
  mu_mailbox_close (mbox);
  mu_mailbox_destroy (&mbox);
  mu_debug_destroy (&debug, mach);

  return rc ? 1 : 0;
}

