/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002, 2003 Free Software Foundation, Inc.

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
#include <mailutils/argp.h>
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

const char *program_version = "sieve (" PACKAGE_STRING ")";

static char doc[] =
/* TRANSLATORS: Please, preserve the vertical tabulation (^K character)
   in this message */
N_("GNU sieve -- a mail filtering tool\n\
\v\
Debug flags:\n\
  g - main parser traces\n\
  T - mailutils traces (MU_DEBUG_TRACE)\n\
  P - network protocols (MU_DEBUG_PROT)\n\
  t - sieve trace (MU_SIEVE_DEBUG_TRACE)\n\
  i - sieve instructions trace (MU_SIEVE_DEBUG_INSTR)\n");

#define D_DEFAULT "TPt"

#define ARG_LINE_INFO 257

static struct argp_option options[] =
{
  {"no-actions", 'n', 0, 0,
   N_("Do not execute any actions, just print what would be done"), 0},

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

struct options {
  int keep_going;
  int compile_only;
  char *mbox;
  char *tickets;
  int tickets_default;
  int debug_level;
  int sieve_debug;
  int verbose;
  char *script;
};

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

static error_t
parser (int key, char *arg, struct argp_state *state)
{
  struct options *opts = state->input;
  int rc;
  
  switch (key)
    {
    case ARGP_KEY_INIT:
      if (!opts->tickets)
	{
	  opts->tickets = mu_tilde_expansion ("~/.tickets", "/", NULL);
	  opts->tickets_default = 1;
	}
      if (!opts->debug_level)
	opts->debug_level = MU_DEBUG_ERROR;
      log_facility = 0;
      break;

    case 'e':
      rc = mu_set_user_email (arg);
      if (rc)
	argp_error (state, _("Invalid email: %s"), mu_strerror (rc));
      break;
      
    case 'n':
      opts->sieve_debug |= MU_SIEVE_DRY_RUN;
      break;

    case 'k':
      opts->keep_going = 1;
      break;

    case 'c':
      opts->compile_only = 1;
      break;

    case 'D':
      opts->compile_only = 2;
      break;
      
    case 'f':
      if (opts->mbox)
	argp_error (state, _("Only one MBOX can be specified"));
      opts->mbox = strdup (arg);
      break;
      
    case 't':
      free (opts->tickets);
      opts->tickets = mu_tilde_expansion (arg, "/", NULL);
      opts->tickets_default = 0;
      break;
      
    case 'd':
      if (!arg)
	arg = D_DEFAULT;
      for (; *arg; arg++)
	{
	  switch (*arg)
	    {
	    case 'T':
	      opts->debug_level |= MU_DEBUG_TRACE;
	      break;

	    case 'P':
	      opts->debug_level |= MU_DEBUG_PROT;
	      break;

	    case 'g':
	      sieve_yydebug = 1;
	      break;

	    case 't':
	      opts->sieve_debug |= MU_SIEVE_DEBUG_TRACE;
	      break;
	      
	    case 'i':
	      opts->sieve_debug |= MU_SIEVE_DEBUG_INSTR;
	      break;
	      
	    default:
	      argp_error (state, _("%c is not a valid debug flag"), *arg);
	      break;
	    }
	}
      break;

    case 'v':
      opts->verbose = 1;
      break;

    case ARG_LINE_INFO:
      sieve_print_locus = is_true_p (arg);
      break;
      
    case ARGP_KEY_ARG:
      if (opts->script)
	argp_error (state, _("Only one SCRIPT can be specified"));
      opts->script = mu_tilde_expansion (arg, "/", NULL);
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

static const char *sieve_argp_capa[] =
{
  "common",
  "mailbox",
  "license",
  "logging",
  "mailer",
  "sieve",
#ifdef WITH_TLS
  "tls",
#endif
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

static int
stderr_debug_print (mu_debug_t unused, size_t level, const char *fmt,
		    va_list ap)
{
  vfprintf ((level == MU_DEBUG_ERROR) ? stderr : stdout, fmt, ap);
  return 0;
}

static int
syslog_debug_print (mu_debug_t unused, size_t level, const char *fmt,
		    va_list ap)
{
  vsyslog ((level == MU_DEBUG_ERROR) ? LOG_ERR : LOG_DEBUG, fmt, ap);
  return 0;
}

static void
stdout_action_log (void *unused,
		   const sieve_locus_t *locus, size_t msgno, message_t msg,
		   const char *action, const char *fmt, va_list ap)
{
  size_t uid = 0;
  
  message_get_uid (msg, &uid);

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
		   const sieve_locus_t *locus, size_t msgno, message_t msg,
		   const char *action, const char *fmt, va_list ap)
{
  size_t uid = 0;
  char *text = NULL;
  
  message_get_uid (msg, &uid);

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
  sieve_machine_t mach;
  wicket_t wicket = 0;
  ticket_t ticket = 0;
  mu_debug_t debug = 0;
  mailbox_t mbox = 0;
  int rc;
  struct options opts = {0};
  int (*debugfp) __P ((mu_debug_t, size_t level, const char *, va_list));

  /* Native Language Support */
  mu_init_nls ();

  mu_argp_init (program_version, NULL);
#ifdef WITH_TLS
  mu_tls_init_client_argp ();
#endif
  sieve_argp_init ();
  rc = mu_argp_parse (&argp, &argc, &argv, ARGP_IN_ORDER, sieve_argp_capa,
		      0, &opts);

  if (rc)
    return 1;

  mu_register_all_formats ();

  /* Sieve interpreter setup. */
  rc = sieve_machine_init (&mach, NULL);
  if (rc)
    {
      mu_error (_("Cannot initialize sieve machine: %s"), mu_strerror (rc));
      return 1;
    }

  if (log_facility)
    {
      openlog ("sieve", LOG_PID, log_facility);
      mu_error_set_print (mu_syslog_error_printer);
      sieve_set_debug (mach, sieve_syslog_debug_printer);
      if (opts.verbose)
	sieve_set_logger (mach, syslog_action_log);
      debugfp = syslog_debug_print;
    }
  else
    {
      sieve_set_debug (mach, sieve_stderr_debug_printer);
      if (opts.verbose)
	sieve_set_logger (mach, stdout_action_log);
      debugfp = stderr_debug_print;
    }
  
  rc = sieve_compile (mach, opts.script);
  if (rc)
    return 1;

  /* We can finish if its only a compilation check. */
  if (opts.compile_only)
    {
      if (opts.compile_only == 2)
	sieve_disass (mach);
      return 0;
    }

  /* Create a ticket, if we can. */
  if (opts.tickets)
    {
      if ((rc = wicket_create (&wicket, opts.tickets)) == 0)
        {
          if ((rc = wicket_get_ticket (wicket, &ticket, 0, 0)) != 0)
            {
              mu_error (_("ticket_get failed: %s"), mu_strerror (rc));
              goto cleanup;
            }
        }
      else if (!(opts.tickets_default && errno == ENOENT))
        {
          mu_error (_("wicket_create <%s> failed: %s"),
                    opts.tickets, mu_strerror (rc));
          goto cleanup;
        }
      if (ticket)
        sieve_set_ticket (mach, ticket);
    }

  /* Create a debug object, if needed. */
  if (opts.debug_level)
    {
      if ((rc = mu_debug_create (&debug, mach)))
	{
	  mu_error (_("mu_debug_create failed: %s"), mu_strerror (rc));
	  goto cleanup;
	}
      if ((rc = mu_debug_set_level (debug, opts.debug_level)))
	{
	  mu_error (_("mu_debug_set_level failed: %s"),
		    mu_strerror (rc));
	  goto cleanup;
	}
      if ((rc = mu_debug_set_print (debug, debugfp, mach)))
	{
	  mu_error (_("mu_debug_set_print failed: %s"),
		    mu_strerror (rc));
	  goto cleanup;
	}
    }
  
  sieve_set_debug_level (mach, debug, opts.sieve_debug);
    
  /* Create, give a ticket to, and open the mailbox. */
  if ((rc = mailbox_create_default (&mbox, opts.mbox)) != 0)
    {
      mu_error (_("mailbox_create <%s> failed: %s"),
	       opts.mbox ? opts.mbox : _("default"), mu_strerror (rc));
      goto cleanup;
    }

  if (debug && (rc = mailbox_set_debug (mbox, debug)))
    {
      mu_error (_("mailbox_set_debug failed: %s"), mu_strerror (rc));
      goto cleanup;
    }

  if (ticket)
    {
      folder_t folder = NULL;
      authority_t auth = NULL;

      if ((rc = mailbox_get_folder (mbox, &folder)))
	{
	  mu_error (_("mailbox_get_folder failed: %s"),
		   mu_strerror (rc));
	  goto cleanup;
	}

      if ((rc = folder_get_authority (folder, &auth)))
	{
	  mu_error (_("folder_get_authority failed: %s"),
		   mu_strerror (rc));
	  goto cleanup;
	}

      /* Authentication-less folders don't have authorities. */
      if (auth && (rc = authority_set_ticket (auth, ticket)))
	{
	  mu_error (_("authority_set_ticket failed: %s"),
		   mu_strerror (rc));
	  goto cleanup;
	}
    }

  /* Open the mailbox read-only if we aren't going to modify it. */
  if (opts.sieve_debug & MU_SIEVE_DRY_RUN)
    rc = mailbox_open (mbox, MU_STREAM_READ);
  else
    rc = mailbox_open (mbox, MU_STREAM_RDWR);

  if (rc != 0)
    {
      mu_error (_("Opening mailbox `%s' failed: %s"),
		opts.mbox ? opts.mbox : _("default"), mu_strerror (rc));
      mailbox_destroy (&mbox);
      goto cleanup;
    }

  /* Process the mailbox */
  rc = sieve_mailbox (mach, mbox);

cleanup:
  if (mbox && !(opts.sieve_debug & MU_SIEVE_DRY_RUN))
    {
      int e;

      /* A message won't be marked deleted unless the script executed
         succesfully on it, so we always do an expunge, it will delete
         any messages that were marked DELETED even if execution failed
         on a later message. */
      if ((e = mailbox_expunge (mbox)) != 0)
	mu_error (_("Expunge on mailbox `%s' failed: %s"),
		  opts.mbox ? opts.mbox : _("default"), mu_strerror (e));

      if (e && !rc)
	rc = e;
    }

  sieve_machine_destroy (&mach);
  mailbox_close (mbox);
  mailbox_destroy (&mbox);
  mu_debug_destroy (&debug, mach);

  return rc ? 1 : 0;
}

