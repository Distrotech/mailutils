/*

sieve script interpreter.

*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <argp.h>

#include "sieve.h"

#include <mailutils/registrar.h>
#include <mailutils/mailbox.h>

void mutil_register_all_mbox_formats(void);

const char *argp_program_version = "sieve (" PACKAGE ") " VERSION;
const char *argp_program_bug_address = "<bug-mailutils@gnu.org>";

static char doc[] =
  "GNU sieve -- a mail filtering tool\n"
  "\v"
  "Debug flags:\n"
  "  T - mailutil traces (MU_DEBUG_TRACE)\n"
  "  P - network protocols (MU_DEBUG_PROT)\n"
  "  t - sieve trace (SV_DEBUG_TRACE)\n"
  "  h - sieve header filling (SV_DEBUG_HDR_FILL)\n"
  "  q - sieve message queries (SV_DEBUG_MSG_QUERY)\n"
  ;

static struct argp_option options[] = {
  {"no-actions", 'n', 0, 0,
   "No actions executed, just print what would be done", 0},

  {"keep-going", 'k', 0, 0,
   "Keep on going if execution fails on a message", 0},

  {"compile-only", 'c', 0, 0,
   "Compile script and exit", 0},

  {"mbox-url", 'f', "MBOX", 0,
   "Mailbox to sieve (defaults to user's mail spool)", 0},

  {"ticket", 't', "TICKET", 0,
   "Ticket file for mailbox authentication", 0},

  {"mailer-url", 'm', "MAILER", 0,
   "Mailer URL (defaults to \"sendmail:\")", 0},

  {"debug", 'd', "FLAGS", OPTION_ARG_OPTIONAL,
   "Debug flags (defaults to \"TPt\")", 0},

  {0}
};

struct options
{
  int no_actions;
  int keep_going;
  int compile_only;
  char *mbox;
  char *tickets;
  int debug_level;
  char *mailer;
  char *script;
};

static error_t
parser (int key, char *arg, struct argp_state *state)
{
  struct options *opts = state->input;

  switch (key)
    {
    case ARGP_KEY_INIT:
      opts->mailer = "sendmail:";
      opts->debug_level = MU_DEBUG_ERROR;
      break;
    case 'n':
      opts->no_actions = SV_FLAG_NO_ACTIONS;
      break;
    case 'k':
      opts->keep_going = 1;
      break;
    case 'c':
      opts->compile_only = 1;
      break;
    case 'f':
      opts->mbox = arg;
      break;
    case 't':
      opts->tickets = arg;
      break;
    case 'd':
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
	    case 't':
	      opts->debug_level |= SV_DEBUG_TRACE;
	      break;
	    case 'h':
	      opts->debug_level |= SV_DEBUG_HDR_FILL;
	      break;
	    case 'q':
	      opts->debug_level |= SV_DEBUG_MSG_QUERY;
	      break;
	    default:
	      argp_error (state, "%c is not a valid debug flag", *arg);
	      break;
	    }
	}
      break;

    case ARGP_KEY_ARG:
      if (opts->script)
	argp_error (state, "only one SCRIPT can be specified");
      opts->script = arg;
      break;

    case ARGP_KEY_NO_ARGS:
      argp_error (state, "SCRIPT must be specified");
      break;

    default:
      return ARGP_ERR_UNKNOWN;
    }

  return 0;
}

static struct argp argp = {
  options,
  parser,
  "SCRIPT",
  doc
};

static void
parse_error (const char *script, int lineno, const char *errmsg)
{
  fprintf (stderr, "%s:%d: %s\n", script, lineno, errmsg);
}

static void
execute_error (const char *script, message_t msg, int rc, const char *errmsg)
{
  size_t uid = 0;
  message_get_uid (msg, &uid);

  if (rc)
    fprintf (stderr, "%s on msg uid %d failed: %s\n", script, uid, errmsg);
  else
    fprintf (stderr, "%s on msg uid %d failed: %s (%s)\n", script, uid,
	     errmsg, strerror (rc));
}

static void
action_log (const char *script, message_t msg, const char *action,
	    const char *fmt, va_list ap)
{
  size_t uid = 0;

  message_get_uid (msg, &uid);

  fprintf (stdout, "%s on msg uid %d", action, uid);
  if (strlen (fmt))
    {
      fprintf (stdout, ": ");
      vfprintf (stdout, fmt, ap);
    }
  fprintf (stdout, "\n");
}

static int
debug_print (mu_debug_t debug, size_t level, const char *fmt, va_list ap)
{
  (void) debug;
  vfprintf ((level == MU_DEBUG_ERROR) ? stderr : stdout, fmt, ap);
  return 0;
}

int
main (int argc, char *argv[])
{
  sv_interp_t interp = 0;
  sv_script_t script = 0;
  wicket_t wicket = 0;
  ticket_t ticket = 0;
  mu_debug_t debug = 0;
  mailer_t mailer = 0;
  mailbox_t mbox = 0;

  struct options opts = { 0 };

  size_t count = 0;
  int msgno = 0;

  int rc = 0;

  argp_parse (&argp, argc, argv, ARGP_IN_ORDER, NULL, &opts);

  mutil_register_all_mbox_formats ();

  /* Sieve interpreter setup. */
  rc = sv_interp_alloc (&interp, parse_error, execute_error, action_log);

  if (rc)
    {
      fprintf (stderr, "sv_interp_alloc() failed: %s\n", sv_strerror (rc));
      goto cleanup;
    }

  rc = sv_script_parse (&script, interp, opts.script);

  if (rc)
    {
      /* Don't print this if there was a parse failure, because
         parse_error() was already called to report it. */
      if (rc != SV_EPARSE)
	fprintf (stderr, "parsing %s failed: %s\n",
		 opts.script, sv_strerror (rc));
      goto cleanup;
    }

  /* We can finish if its only a compilation check. */
  if (opts.compile_only)
    goto cleanup;

  /* Create a ticket, if we can. */
  if (opts.tickets)
    {
      if ((rc = wicket_create (&wicket, opts.tickets)) != 0)
	{
	  fprintf (stderr, "wicket create <%s> failed: %s\n",
		   opts.tickets, strerror (rc));
	  goto cleanup;
	}
      if ((rc = wicket_get_ticket (wicket, &ticket, 0, 0)) != 0)
	{
	  fprintf (stderr, "ticket get failed: %s\n", strerror (rc));
	  goto cleanup;
	}
    }

  /* Create a debug object, if needed. */
  if (opts.debug_level)
    {
      if ((rc = mu_debug_create (&debug, interp)))
	{
	  fprintf (stderr, "mu_debug_create failed: %s\n", strerror (rc));
	  goto cleanup;
	}
      if ((rc = mu_debug_set_level (debug, opts.debug_level)))
	{
	  fprintf (stderr, "mu_debug_set_level failed: %s\n", strerror (rc));
	  goto cleanup;
	}
      if ((rc = mu_debug_set_print (debug, debug_print, interp)))
	{
	  fprintf (stderr, "mu_debug_set_print failed: %s\n", strerror (rc));
	  goto cleanup;
	}
    }

  /* Create a mailer. */
  if ((rc = mailer_create(&mailer, opts.mailer)))
  {
      fprintf (stderr, "mailer create <%s> failed: %s\n",
	       opts.mailer, strerror (rc));
      goto cleanup;
  }
  if (debug && (rc = mailer_set_debug (mailer, debug)))
    {
      fprintf (stderr, "mailer_set_debug failed: %s\n", strerror (rc));
      goto cleanup;
    }

  /* Create, give a ticket to, and open the mailbox. */
  if ((rc = mailbox_create_default (&mbox, opts.mbox)) != 0)
    {
      fprintf (stderr, "mailbox create <%s> failed: %s\n",
	       opts.mbox ? opts.mbox : "default", strerror (rc));
      goto cleanup;
    }
  
  if (debug && (rc = mailbox_set_debug (mbox, debug)))
    {
      fprintf (stderr, "mailbox_set_debug failed: %s\n", strerror (rc));
      goto cleanup;
    }

  if (ticket)
    {
      folder_t folder = NULL;
      authority_t auth = NULL;

      if ((rc = mailbox_get_folder (mbox, &folder)))
	{
	  fprintf (stderr, "mailbox_get_folder failed: %s", strerror (rc));
	  goto cleanup;
	}

      if ((rc = folder_get_authority (folder, &auth)))
	{
	  fprintf (stderr, "folder_get_authority failed: %s", strerror (rc));
	  goto cleanup;
	}

      /* Authentication-less folders don't have authorities. */
      if (auth && (rc = authority_set_ticket (auth, ticket)))
	{
	  fprintf (stderr, "authority_set_ticket failed: %s", strerror (rc));
	  goto cleanup;
	}
    }

  /* Open the mailbox read-only if we aren't going to modify it. */
  if (opts.no_actions)
    rc = mailbox_open (mbox, MU_STREAM_READ);
  else
    rc = mailbox_open (mbox, MU_STREAM_RDWR);

  if (rc != 0)
    {
      fprintf (stderr, "open on %s failed: %s\n",
	       opts.mbox ? opts.mbox : "default", strerror (rc));
      goto cleanup;
    }

  /* Iterate over all the messages in the mailbox. */
  rc = mailbox_messages_count (mbox, &count);

  if (rc != 0)
    {
      fprintf (stderr, "message count on %s failed: %s\n",
	       opts.mbox ? opts.mbox : "default", strerror (rc));
      goto cleanup;
    }

  for (msgno = 1; msgno <= count; ++msgno)
    {
      message_t msg = 0;

      if ((rc = mailbox_get_message (mbox, msgno, &msg)) != 0)
	{
	  fprintf (stderr, "get message on %s (msg %d) failed: %s\n",
	      opts.mbox ? opts.mbox : "default", msgno, strerror (rc));
	  goto cleanup;
	}

      rc = sv_script_execute (script, msg, ticket, debug, mailer, opts.no_actions);

      if (rc)
	{
	  fprintf (stderr, "execution of %s on %s (msg %d) failed: %s\n",
		   opts.script, opts.mbox ? opts.mbox : "default", msgno,
		   sv_strerror (rc));
	  if (opts.keep_going)
	    rc = 0;
	  else
	    goto cleanup;
	}
    }

cleanup:
  if (mbox && !opts.no_actions && !opts.compile_only)
    {
      int e;

      /* A message won't be marked deleted unless the script executed
	 succesfully on it, so we always do an expunge, it will delete
	 any messages that were marked DELETED even if execution failed
	 on a later message. */
      if ((e = mailbox_expunge (mbox)) != 0)
	fprintf (stderr, "expunge on %s failed: %s\n",
	    opts.mbox ? opts.mbox : "default", strerror (e));

      if(e && !rc)
	rc = e;
    }

  mailbox_close (mbox);
  mailbox_destroy (&mbox);
  mu_debug_destroy(&debug, interp);
  sv_script_free (&script);
  sv_interp_free (&interp);

  return rc ? 1 : 0;
}

void
mutil_register_all_mbox_formats (void)
{
  list_t bookie = 0;
  registrar_get_list (&bookie);
  list_append (bookie, path_record);
  list_append (bookie, file_record);
  list_append (bookie, mbox_record);
  list_append (bookie, pop_record);
  list_append (bookie, imap_record);
  list_append (bookie, mh_record);
  list_append (bookie, sendmail_record);
  list_append (bookie, smtp_record);
}

