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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <sys/stat.h>
#include <sys/types.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <mailutils/address.h>
#include <mailutils/argp.h>
#include <mailutils/debug.h>
#include <mailutils/errno.h>
#include <mailutils/list.h>
#include <mailutils/mailer.h>
#include <mailutils/mutil.h>
#include <mailutils/message.h>
#include <mailutils/registrar.h>
#include <mailutils/stream.h>

const char *argp_program_version = "mail.remote (" PACKAGE ") " VERSION;
const char *argp_program_bug_address = "<bug-mailutils@gnu.org>";
static char doc[] =
  "GNU mail.remote -- pseudo-sendmail interface for mail delivery\n"
  "\v"
  "\n"
  "An RFC2822 formatted message is read from stdin and delivered using\n"
  "the mailer. This utility can be used as a drop-in replacement\n"
  "for /bin/sendmail to forward mail directly to an SMTP gateway.\n"
  "\n"
  "The default mailer is \"sendmail:\", which is not supported by this\n"
  "utility (it is intended to be used when you don't have a working\n"
  "sendmail). You should specify your SMTP gateway by specifying\n"
  "a --mailer as something like \"smtp://mail.example.com\". This would\n"
  "normally be added to your user-specific configuration file,\n"
  "  ~/.mailutils/mailutils,\n"
  "or the global configuration file,\n"
  "  /etc/mailutils.rc,\n"
  "with a line such as:\n"
  "  :mailer --mailer=smtp://mail.example.com\n"
  "\n"
  "If not explicitly specified, the default from address is derived from the\n"
  "\"From:\" field in the message, if present, or the default user's email\n"
  "address if not present.\n"
  "\n"
  "If not explicitly specified, the default to addresses are derived from the\n"
  "\"To:\", \"Cc:\", and \"Bcc:\" fields in the message.\n"
  "\n"
  "If --debug is specified, the envelope commands in the SMTP protocol\n"
  "transaction will be printed to stdout. If specified more than once,\n"
  "the data part of the protocol transaction will also be printed to stdout.\n"
  ;

static struct argp_option options[] = {
  {"from",  'f', "ADDR", 0, "Override the default from address\n"},
  {"debug", 'd', NULL,   0, "Enable debugging output"},
  {      0, 'o', "OPT",  OPTION_HIDDEN, "Ignored for sendmail compatibility"},
  {0}
};

static int optdebug;
static const char* optfrom;

static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  switch (key)
    {
    case 'f':
      optfrom = arg;
      break;

    case 'd':
      optdebug++;
      break;

    case 'o':
      break;

    default: 
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

static struct argp argp = {
  options,
  parse_opt,
  "[TO-ADDR]...",
  doc,
};

static const char *capa[] = {
  "common",
  "mailer",
  "address",
  "license",
  NULL
};

int
main (int argc, char **argv)
{
  int status = 0;
  int optind = 0;

  stream_t in = 0;
  message_t msg = 0;
  mailer_t mailer = 0;
  address_t from = 0;
  address_t to = 0;

  int mailer_flags = 0;

  /* Register mailers. */
  {
    list_t bookie;
    registrar_get_list (&bookie);
    list_append (bookie, smtp_record);
  }

  mu_argp_parse (&argp, &argc, &argv, 0, capa, &optind, NULL);

  if (optfrom)
    {
      if ((status = address_create (&from, optfrom)))
	{
	  fprintf (stderr, "Parsing from addresses failed: %s\n",
		   mu_errstring (status));
	  goto end;
	}
    }

  if (argv[optind])
    {
      char **av = argv + optind;

      if ((status = address_createv (&to, (const char **) av, -1)))
	{
	  fprintf (stderr, "Parsing to addresses failed: %s\n",
		   mu_errstring (status));
	  goto end;
	}
    }

  if ((status = stdio_stream_create (&in, stdin, 0)))
    {
      fprintf (stderr, "Failed: %s\n", mu_errstring (status));
      goto end;
    }

  if ((status = stream_open (in)))
    {
      fprintf (stderr, "Opening stdin failed: %s\n", mu_errstring (status));
      goto end;
    }

  if ((status = message_create (&msg, NULL)))
    {
      fprintf (stderr, "Failed: %s\n", mu_errstring (status));
      goto end;
    }

  if ((status = message_set_stream (msg, in, NULL)))
    {
      fprintf (stderr, "Failed: %s\n",
	       mu_errstring (status));
      goto end;
    }

  if ((status = mailer_create (&mailer, NULL)))
    {
      const char *url = NULL;
      mailer_get_url_default (&url);
      fprintf (stderr, "Creating mailer '%s' failed: %s\n",
	  url, mu_errstring (status));
      goto end;
    }

  if (optdebug)
    {
      mu_debug_t debug;
      mailer_get_debug (mailer, &debug);
      mu_debug_set_level (debug, MU_DEBUG_TRACE | MU_DEBUG_PROT);

      if (optdebug > 1)
	mailer_flags = MAILER_FLAG_DEBUG_DATA;
    }

  if ((status = mailer_open (mailer, mailer_flags)))
    {
      const char *url = NULL;
      mailer_get_url_default (&url);
      fprintf (stderr, "Opening mailer '%s' failed: %s\n",
	  url, mu_errstring (status));
      goto end;
    }

  if ((status = mailer_send_message (mailer, msg, from, to)))
    {
      fprintf (stderr, "Sending message failed: %s\n", mu_errstring (status));
      goto end;
    }

  if ((status = mailer_close (mailer)))
    {
      fprintf (stderr, "Closing mailer failed: %s\n", mu_errstring (status));
      goto end;
    }

end:

  address_destroy (&from);
  address_destroy (&to);
  stream_destroy (&in, NULL);
  mailer_destroy (&mailer);

  return status ? 1 : 0;
}
