/*

sieve interpreter

*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>

#include "sieve.h"

#include <mailutils/registrar.h>
#include <mailutils/mailbox.h>

void mutil_register_all_mbox_formats(void);

const char USAGE[] =
  "usage: sieve [-hnkvc] [-d <TPthq>] [-f mbox] [-t tickets] script\n";

const char HELP[] =
  "  -h   print this helpful message and exit.\n"
  "  -n   no execute, just print actions.\n"
  "  -k   keep on going if execution fails on a message.\n"
  "  -v   print actions executed on a message.\n"
  "  -c   compile script and exit.\n"
  "  -f   the mbox to sieve, defaults to the users spool file.\n"
  "  -t   a ticket file to use for authentication to mailboxes.\n"
  "  -d   debug flags, each turns on a different set of messages:\n"
  "         T - mailutil traces\n"
  "         P - network protocols\n"
  "         t - sieve trace\n"
  "         h - sieve header parseing\n"
  "         q - sieve message queries\n"
  "  -m   a mailer URL (default is \"sendmail:\")\n"
  ;

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
debug_print (mu_debug_t debug, const char *fmt, va_list ap)
{
  (void) debug;
  vfprintf (stdout, fmt, ap);
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

  size_t count = 0;
  int msgno = 0;

  int rc = 0;

  int opt_no_actions = 0;
  int opt_keep_going = 0;
  int opt_compile_only = 0;
  char *opt_mbox = 0;
  char *opt_tickets = 0;
  int opt_debug_level = 0;
  char *opt_mailer = "sendmail:";
  char *opt_script = 0;

  int opt;

  mutil_register_all_mbox_formats ();

  while ((opt = getopt (argc, argv, "hnkcf:t:d:")) != -1)
    {
      switch (opt)
	{
	case 'h':
	  printf ("%s\n", USAGE);
	  printf ("%s", HELP);
	  return 0;
	case 'n':
	  opt_no_actions = SV_FLAG_NO_ACTIONS;
	  break;
	case 'k':
	  opt_keep_going = 1;
	  break;
	case 'c':
	  opt_compile_only = 1;
	  break;
	case 'f':
	  opt_mbox = optarg;
	  break;
	case 't':
	  opt_tickets = optarg;
	  break;
	case 'd':
	  for (; *optarg; optarg++)
	    {
	      switch (*optarg)
		{
		case 'T':
		  opt_debug_level |= MU_DEBUG_TRACE;
		  break;
		case 'P':
		  opt_debug_level |= MU_DEBUG_PROT;
		  break;
		case 't':
		  opt_debug_level |= SV_DEBUG_TRACE;
		  break;
		case 'h':
		  opt_debug_level |= SV_DEBUG_HDR_FILL;
		  break;
		case 'q':
		  opt_debug_level |= SV_DEBUG_MSG_QUERY;
		  break;
		default:
		  fprintf (stderr, "unknown debug flag %c\n", *optarg);
		  fprintf (stderr, "%s", USAGE);
		  return 1;
		}
	    }
	  break;
	default:
	  fprintf (stderr, "%s", USAGE);
	  return 1;
	}
    }

  if (!argv[optind])
    {
      fprintf (stderr, "%s", USAGE);
      return 1;
    }

  opt_script = argv[optind];

  /* Sieve interpreter setup. */
  rc = sv_interp_alloc (&interp, parse_error, execute_error, action_log);

  if (rc)
    {
      fprintf (stderr, "sv_interp_alloc() failed: %s\n", sv_strerror (rc));
      goto cleanup;
    }

  rc = sv_script_parse (&script, interp, opt_script);

  if (rc)
    {
      /* Don't print this if there was a parse failure, because
         parse_error() was already called to report it. */
      if (rc != SV_EPARSE)
	fprintf (stderr, "parsing %s failed: %s\n",
		 opt_script, sv_strerror (rc));
      goto cleanup;
    }

  /* We can finish if its only a compilation check. */
  if (opt_compile_only)
    goto cleanup;

  /* Create a ticket, if we can. */
  if (opt_tickets)
    {
      if ((rc = wicket_create (&wicket, opt_tickets)) != 0)
	{
	  fprintf (stderr, "wicket create <%s> failed: %s\n",
		   opt_tickets, strerror (rc));
	  goto cleanup;
	}
      if ((rc = wicket_get_ticket (wicket, &ticket, 0, 0)) != 0)
	{
	  fprintf (stderr, "ticket get failed: %s\n", strerror (rc));
	  goto cleanup;
	}
    }

  /* Create a debug object, if needed. */
  if (opt_debug_level)
    {
      if ((rc = mu_debug_create (&debug, interp)))
	{
	  fprintf (stderr, "mu_debug_create failed: %s\n", strerror (rc));
	  goto cleanup;
	}
      if ((rc = mu_debug_set_level (debug, opt_debug_level)))
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
  if ((rc = mailer_create(&mailer, opt_mailer)))
  {
      fprintf (stderr, "mailer create <%s> failed: %s\n",
	       opt_mailer, strerror (rc));
      goto cleanup;
  }
  if (debug && (rc = mailer_set_debug (mailer, debug)))
    {
      fprintf (stderr, "mailer_set_debug failed: %s\n", strerror (rc));
      goto cleanup;
    }

  /* Create, give a ticket to, and open the mailbox. */
  if ((rc = mailbox_create_default (&mbox, opt_mbox)) != 0)
    {
      fprintf (stderr, "mailbox create <%s> failed: %s\n",
	       opt_mbox ? opt_mbox : "default", strerror (rc));
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
  if (opt_no_actions)
    rc = mailbox_open (mbox, MU_STREAM_READ);
  else
    rc = mailbox_open (mbox, MU_STREAM_RDWR);

  if (rc != 0)
    {
      fprintf (stderr, "open on %s failed: %s\n",
	       opt_mbox ? opt_mbox : "default", strerror (rc));
      goto cleanup;
    }

  /* Iterate over all the messages in the mailbox. */
  rc = mailbox_messages_count (mbox, &count);

  if (rc != 0)
    {
      fprintf (stderr, "message count on %s failed: %s\n",
	       opt_mbox ? opt_mbox : "default", strerror (rc));
      goto cleanup;
    }

/*
  if (opt_verbose)
    {
      fprintf (stderr, "mbox %s has %d messages...\n",
	  ic.opt_mbox, count);
    }
*/

  for (msgno = 1; msgno <= count; ++msgno)
    {
      message_t msg = 0;

      if ((rc = mailbox_get_message (mbox, msgno, &msg)) != 0)
	{
	  fprintf (stderr, "get message on %s (msg %d) failed: %s\n",
	      opt_mbox ? opt_mbox : "default", msgno, strerror (rc));
	  goto cleanup;
	}

      rc = sv_script_execute (script, msg, ticket, debug, mailer, opt_no_actions);

      if (rc)
	{
	  fprintf (stderr, "execution of %s on %s (msg %d) failed: %s\n",
		   opt_script, opt_mbox ? opt_mbox : "default", msgno,
		   sv_strerror (rc));
	  if (opt_keep_going)
	    rc = 0;
	  else
	    goto cleanup;
	}
    }

cleanup:
  if (mbox && !opt_no_actions && !opt_compile_only)
    {
      int e;

      /* A message won't be marked deleted unless the script executed
	 succesfully on it, so we always do an expunge, it will delete
	 any messages that were marked DELETED even if execution failed
	 on a later message. */
      if ((e = mailbox_expunge (mbox)) != 0)
	fprintf (stderr, "expunge on %s failed: %s\n",
	    opt_mbox ? opt_mbox : "default", strerror (e));

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

