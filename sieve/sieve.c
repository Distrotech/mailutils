/*
sieve interpreter

The mu_sieve_context needs a parse_error(), execute_error(), and
debug() callbacks. Pass as much as we know into them.

Then we can request trace info on:

  MU_TRACE
  MU_PROT
  MU_SV_HEADER_CACHE
  MU_SV_ACTIONS

*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdarg.h>
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#include "sv.h"

#ifndef EOK
# define EOK 0
#endif

void
sv_printv (sv_interp_ctx_t* ic, int level, const char *fmt, va_list ap)
{
  assert(ic);

  if(level & ic->print_mask)
    vfprintf(ic->print_stream ? ic->print_stream : stdout, fmt, ap);
}

void
sv_print (sv_interp_ctx_t* ic, int level, const char* fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  sv_printv(ic, level, fmt, ap);
  va_end(ap);
}

const char USAGE[] =
  "usage: sieve [-hnvcd] [-D mask] [-f mbox] [-t tickets] script\n";

const char HELP[] =
  "	-h   print this helpful message and exit.\n"
  "	-n   no actions taken, implies -v.\n"
  "	-v   verbose, print actions taken, more v's, more verbose.\n"
  "	-c   compile script but don't run it against an mbox.\n"
  "	-f   the mbox to sieve, defaults to the users spool file.\n"
  "	-t   a ticket file to use for authentication to mailboxes.\n"
  "	-d   daemon mode, sieve mbox, then go into background and sieve.\n"
  "	     new message as they are delivered to mbox.\n"
  "	-D   debug mask, see source for meaning (sorry).\n"
  ;

int
main (int argc, char *argv[])
{
  list_t    bookie = 0;
  mailbox_t mbox = 0;
  wicket_t  wicket = 0;

  sieve_interp_t *interp = 0;
  sieve_script_t *script = 0;

  sv_interp_ctx_t ic = { 0, };
  sv_script_ctx_t sc = { 0, };

  size_t count = 0;
  int i = 0;

  int res = 0;
  FILE *f = 0;

  int opt;

  while ((opt = getopt (argc, argv, "hnvcf:t:dD:")) != -1)
    {
      switch (opt)
	{
	case 'h':
	  printf ("%s\n%s", USAGE, HELP);
	  return 0;
	case 'n':
	  ic.opt_no_actions = 1;
          /* and increase verbosity... */
	case 'v':
	  ic.opt_verbose++;
	  break;
	case 'c':
	  ic.opt_no_run = 1;
	  break;
	case 'f':
	  ic.opt_mbox = optarg;
	  break;
	case 't':
	  ic.opt_tickets = optarg;
	  break;
	case 'd':
	  ic.opt_watch = 1;
	  break;
	case 'D':
          ic.print_mask |= strtoul(optarg, 0, 0);
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
  ic.opt_script = argv[optind];

  switch(ic.opt_verbose)
    {
      case 0: break;
      case 1: break;
      case 2: ic.print_mask |= SV_PRN_ACT; break;
      default: ic.print_mask = ~0; break;
    }

  if (ic.opt_tickets)
  {
    if ((res = wicket_create (&wicket, ic.opt_tickets)) != 0)
    {
      fprintf (stderr, "wicket create <%s> failed: %s\n",
	       ic.opt_tickets, strerror (res));
      return 1;
    }
     if ((res = wicket_get_ticket (wicket, &ic.ticket, 0, 0)) != 0)
    {
      fprintf (stderr, "ticket get failed: %s\n", strerror (res));
      return 1;
    }
  }
    
  registrar_get_list (&bookie);
  list_append (bookie, path_record);
  list_append (bookie, file_record);
  list_append (bookie, mbox_record);
  list_append (bookie, pop_record);
  list_append (bookie, imap_record);

  if (!ic.opt_no_run && (res = mailbox_create_default (&mbox, ic.opt_mbox)) != 0)
    {
      fprintf (stderr, "mailbox create <%s> failed: %s\n",
	       ic.opt_mbox ? ic.opt_mbox : "default", strerror (res));
      return 1;
    }

  if (ic.print_mask & SV_PRN_MU)
    {
      if ((res = mu_debug_create(&ic.debug, &ic)))
	{
	  fprintf (stderr, "mu_debug_create failed: %s\n", strerror(res));
	  return 1;
	}
      if ((res = mu_debug_set_level (ic.debug, MU_DEBUG_TRACE | MU_DEBUG_PROT)))
	{
	  fprintf (stderr, "mu_debug_set_level failed: %s\n", strerror(res));
	  return 1;
	}
      if ((res = mu_debug_set_print (ic.debug, sv_mu_debug_print, &ic)))
	{
	  fprintf (stderr, "mu_debug_set_print failed: %s\n", strerror(res));
	  return 1;
	}
      mailbox_set_debug (mbox, ic.debug);
    }

  if (ic.ticket)
    {
      folder_t folder = NULL;
      authority_t auth = NULL;

      if ((res = mailbox_get_folder (mbox, &folder)))
      {
	fprintf (stderr, "mailbox_get_folder failed: %s", strerror(res));
	return 1;
      }

      if ((res = folder_get_authority (folder, &auth)))
      {
	fprintf (stderr, "folder_get_authority failed: %s", strerror(res));
	return 1;
      }

      if ((res = authority_set_ticket (auth, ic.ticket)))
      {
	fprintf (stderr, "authority_set_ticket failed: %s", strerror(res));
	return 1;
      }
    }

  if (ic.opt_no_actions)
    res = mailbox_open (mbox, MU_STREAM_READ);
  else
    res = mailbox_open (mbox, MU_STREAM_RDWR);

  if (res != 0)
    {
      fprintf (stderr, "mailbox open <%s> failed: %s\n",
	       ic.opt_mbox ? ic.opt_mbox : "default", strerror (res));
      exit (1);
    }
  res = sieve_interp_alloc (&interp, &ic);

  if (res != SIEVE_OK)
    {
      fprintf (stderr, "sieve_interp_alloc() returns %d\n", res);
      return 1;
    }
  res = sv_register_callbacks (interp);

  f = fopen (ic.opt_script, "r");

  if (!f)
    {
      printf ("fopen %s failed: %s\n", ic.opt_script, strerror (errno));
      return 1;
    }
  res = sieve_script_parse (interp, f, &sc, &script);
  if (res != SIEVE_OK)
    {
      printf ("script parse failed: %s\n", sieve_errstr (res));
      return 1;
    }
  fclose (f);

  if (ic.opt_no_run)
    return 0;

  mailbox_messages_count (mbox, &count);
  if (ic.opt_verbose)
    {
      fprintf (stderr, "mbox %s has %d messages...\n",
	  ic.opt_mbox, count);
    }
  for (i = 1; i <= count; ++i)
    {
      sv_msg_ctx_t mc = { 0, };

      mc.mbox = mbox;

      if ((res = mailbox_get_message (mbox, i, &mc.msg)) != 0)
	{
	  fprintf (stderr, "mailbox_get_message(msg %d): %s\n", i,
		   strerror (res));
	  exit (1);
	}
      sv_print (&ic, SV_PRN_ACT, "For message %d:\n", i);

      res = sieve_execute_script (script, &mc);

      if (res != SIEVE_OK)
	{
	  printf ("sieve_execute_script(msg %d) failed: %s (because %s)\n",
		  i, sieve_errstr(res), strerror (mc.rc));
	  exit (1);
	}
      sv_field_cache_release (&mc.cache);
      free (mc.summary);
    }
  res = sieve_script_free (&script);

  if (res != SIEVE_OK)
    {
      printf ("sieve_script_free() returns %d\n", res);
      exit (1);
    }
  res = sieve_interp_free (&interp);

  if (res != SIEVE_OK)
    {
      printf ("sieve_interp_free() returns %d\n", res);
      exit (1);
    }
  if (!ic.opt_no_actions)
    res = mailbox_expunge (mbox);

  if (res != 0)
    {
      printf ("mailbox_expunge failed: %s\n", strerror (res));
      exit (1);
    }
  if (res == 0)
    mailbox_close (mbox);

  mailbox_destroy (&mbox);

  return 0;
}

