/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002, 2004, 
   2005, 2007 Free Software Foundation, Inc.

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
#include <mailutils/property.h>
#include <mailutils/error.h>
#include <mailutils/nls.h>
#include <mailutils/mu_auth.h>

const char *program_version = "mail.remote (" PACKAGE_STRING ")";
static char doc[] =
N_("GNU mail.remote -- pseudo-sendmail interface for mail delivery")
"\v"
N_("This is a simple drop-in replacement for sendmail to forward mail directly\n\
to an SMTP gateway.\n\
You should always specify your SMTP gateway using --mailer option\n\
(the best place to do so is in your configuration file).\n\
\n\
Examples:\n\
\n\
Deliver mail via SMTP gateway at \"mail.example.com\", reading its\n\
contents for recipients of the message.\n\
\n\
   mail.remote --mailer smtp://mail.example.com -t\n\
\n\
Deliver mail only to \"devnull@foo.bar\"\n\
\n\
   mail.remote --mailer smtp://mail.example.com devnull@foo.bar\n\
\n\
Deliver mail to \"devnull@foo.bar\" as well as to the recipients\n\
specified in the message itself:\n\
\n\
   mail.remote --mailer smtp://mail.example.com -t devnull@foo.bar\n");

static struct argp_option options[] = {
  {"from",  'f', N_("ADDR"), 0, N_("Override the default from address")},
  {"read-recipients", 't', NULL, 0, N_("Read message for recipients.") },
  {"debug", 'd', NULL,   0, N_("Print envelope commands in the SMTP protocol transaction. If specified more than once, the data part of the protocol transaction will also be printed.")},
  { NULL,   'o', N_("OPT"), 0, N_("Ignored for sendmail compatibility")},
  { NULL,   'b', N_("OPT"), 0, N_("Ignored for sendmail compatibility")},
  { NULL,   'i', NULL, 0, N_("Ignored for sendmail compatibility")},
  { NULL }
};

static int optdebug;
static const char *optfrom;
static int read_recipients;  /* Read recipients from the message */

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
    case 'b':
    case 'i':
      break;

    case 't':
      read_recipients = 1;
      break;
      
    default: 
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

static struct argp argp = {
  options,
  parse_opt,
  N_("[TO-ADDR]..."),
  doc,
};

static const char *capa[] = {
  "auth",
  "common",
  "mailer",
  "address",
  "license",
  NULL
};

mu_mailer_t mailer;     /* Mailer object */ 
mu_address_t from;      /* Sender address */ 
mu_address_t to;        /* Recipient addresses */
mu_stream_t in;         /* Input stream */

void
mr_exit (int status)
{
  mu_address_destroy (&from);
  mu_address_destroy (&to);
  mu_stream_destroy (&in, NULL);
  mu_mailer_destroy (&mailer);

  exit (status ? 1 : 0);
}

int
main (int argc, char **argv)
{
  int status = 0;
  int optind = 0;

  mu_message_t msg = 0;

  int mailer_flags = 0;

  /* Native Language Support */
  mu_init_nls ();

  /* Register mailers. */
  mu_registrar_record (mu_smtp_record);

  MU_AUTH_REGISTER_ALL_MODULES();
  mu_argp_init (program_version, NULL);
  mu_argp_parse (&argp, &argc, &argv, 0, capa, &optind, NULL);

  if (optfrom)
    {
      if ((status = mu_address_create (&from, optfrom)))
	{
	  mu_error (_("Parsing from addresses failed: %s"),
		    mu_strerror (status));
	  mr_exit (status);
	}
    }

  if (argv[optind])
    {
      if ((status = mu_address_createv (&to, (const char **) (argv + optind), -1)))
	{
	  mu_error (_("Parsing recipient addresses failed: %s"),
		    mu_strerror (status));
	  mr_exit (status);
	}
    }
     
  if ((status = mu_stdio_stream_create (&in, stdin, MU_STREAM_SEEKABLE)))
    {
      mu_error (_("Failed: %s"), mu_strerror (status));
      mr_exit (status);
    }

  if ((status = mu_stream_open (in)))
    {
      mu_error (_("Opening stdin failed: %s"), mu_strerror (status));
      mr_exit (status);
    }

  if ((status = mu_message_create (&msg, NULL)))
    {
      mu_error (_("Failed: %s"), mu_strerror (status));
      mr_exit (status);
    }

  if ((status = mu_message_set_stream (msg, in, NULL)))
    {
      mu_error (_("Failed: %s"), mu_strerror (status));
      mr_exit (status);
    }

  if ((status = mu_mailer_create (&mailer, NULL)))
    {
      const char *url = NULL;
      mu_mailer_get_url_default (&url);
      mu_error (_("Creating mailer `%s' failed: %s"),
		url, mu_strerror (status));
      mr_exit (status);
    }

  if (optdebug)
    {
      mu_debug_t debug;
      mu_mailer_get_debug (mailer, &debug);
      mu_debug_set_level (debug, MU_DEBUG_TRACE | MU_DEBUG_PROT);

      if (optdebug > 1)
	mailer_flags = MAILER_FLAG_DEBUG_DATA;
    }

  if (read_recipients)
    {
      mu_property_t property = NULL;

      mu_mailer_get_property (mailer, &property);
      mu_property_set_value (property, "READ_RECIPIENTS", "true", 1);
    }
  
  if ((status = mu_mailer_open (mailer, mailer_flags)))
    {
      const char *url = NULL;
      mu_mailer_get_url_default (&url);
      mu_error (_("Opening mailer `%s' failed: %s"),
		url, mu_strerror (status));
      mr_exit (status);
    }

  if ((status = mu_mailer_send_message (mailer, msg, from, to)))
    {
      mu_error (_("Sending message failed: %s"), mu_strerror (status));
      mr_exit (status);
    }

  if ((status = mu_mailer_close (mailer)))
    {
      mu_error (_("Closing mailer failed: %s"), mu_strerror (status));
      mr_exit (status);
    }

  mr_exit (status);
}

