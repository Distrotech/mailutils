/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2002, 2003,
   2004 Free Software Foundation, Inc.

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

 /**
  *
  * Created as an example for using mailutils API
  * Sean 'Shaleh' Perry <shaleh@debian.org>, 1999
  * Alain Magloire alainm@gnu.org
  *
  **/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <mailutils/address.h>
#include <mailutils/argp.h>
#include <mailutils/debug.h>
#include <mailutils/errno.h>
#include <mailutils/header.h>
#include <mailutils/list.h>
#include <mailutils/mailbox.h>
#include <mailutils/message.h>
#include <mailutils/registrar.h>
#include <mailutils/stream.h>
#include <mailutils/nls.h>
#include <mailutils/tls.h>
#include <mailutils/mutil.h>
#include <mailutils/error.h>
#include <mailutils/mime.h>

const char *program_version = "from (" PACKAGE_STRING ")";
static char doc[] = N_("GNU from -- display from and subject");

static struct argp_option options[] = {
  {"debug",  'd', NULL,   0, N_("Enable debugging output"), 0},
  {0, 0, 0, 0}
};

static int debug;

static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  switch (key)
    {
    case 'd':
      debug++;
      break;

    default: 
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

static struct argp argp = {
  options,
  parse_opt,
  N_("[URL]"),
  doc,
};

static const char *capa[] = {
  "common",
  "license",
  "mailbox",
#ifdef WITH_TLS
  "tls",
#endif
  NULL
};

static char *
rfc2047_decode_wrapper (char *buf, size_t buflen)
{
  char locale[32];
  char *charset = NULL;
  char *tmp;
  int rc;

  memset (locale, 0, sizeof (locale));

  /* Try to deduce the charset from LC_ALL or LANG variables */

  tmp = getenv ("LC_ALL");
  if (!tmp)
    tmp = getenv ("LANG");

  if (tmp)
    {
      char *sp = NULL;
      char *lang;
      char *terr;

      strncpy (locale, tmp, sizeof (locale) - 1);

      lang = strtok_r (locale, "_", &sp);
      terr = strtok_r (NULL, ".", &sp);
      charset = strtok_r (NULL, "@", &sp);

      if (!charset)
	charset = mu_charset_lookup (lang, terr);
    }

  if (!charset)
    return strdup (buf);

  rc = rfc2047_decode (charset, buf, &tmp);
  if (rc)
    {
      if (debug)
	mu_error (_("Cannot decode line `%s': %s"),
		  buf, mu_strerror (rc));
      return strdup (buf);
    }

  return tmp;
}

int
main (int argc, char **argv)
{
  mailbox_t mbox;
  size_t i;
  size_t count = 0;
  char *mailbox_name = NULL;
  char *buf;
  char personal[128];
  int status;

  /* Native Language Support */
  mu_init_nls ();

  {
    int opt;
    mu_argp_init (program_version, NULL);
#ifdef WITH_TLS
    mu_tls_init_client_argp ();
#endif
    mu_argp_parse (&argp, &argc, &argv, 0, capa, &opt, NULL);
    mailbox_name = argv[opt];
  }

  /* Register the desire formats.  */
  mu_register_all_mbox_formats ();

  status = mailbox_create_default (&mbox, mailbox_name);
  if (status != 0)
    {
      mu_error (_("Could not create mailbox <%s>: %s."),
		mailbox_name ? mailbox_name : _("default"),
		mu_strerror (status));
      exit (1);
    }

  /* Debuging Trace.  */
  if (debug)
  {
    mu_debug_t debug;
    mailbox_get_debug (mbox, &debug);
    mu_debug_set_level (debug, MU_DEBUG_TRACE|MU_DEBUG_PROT);
  }

  status = mailbox_open (mbox, MU_STREAM_READ);
  if (status != 0)
    {
      mu_error (_("Could not open mailbox <%s>: %s."),
		mailbox_name, mu_strerror (status));
      exit (1);
    }

  mailbox_messages_count (mbox, &count);
  for (i = 1; i <= count; ++i)
    {
      message_t msg;
      header_t hdr;
      size_t len = 0;

      if ((status = mailbox_get_message (mbox, i, &msg)) != 0
	  || (status = message_get_header (msg, &hdr)) != 0)
	{
	  mu_error (_("msg %d : %s"), i, mu_strerror (status));
	  exit (2);
	}

      status = header_aget_value (hdr, MU_HEADER_FROM, &buf);
      if (status == 0)
	{
	  address_t address = NULL;

	  char *s = rfc2047_decode_wrapper (buf, strlen (buf));
	  address_create (&address, s);
	  free (s);

	  len = 0;
	  address_get_personal (address, 1, personal, sizeof (personal), &len);
	  printf ("%s\t", (len != 0) ? personal : buf);
	  address_destroy (&address);
	}
      else
	{
	  status = header_aget_value (hdr, MU_HEADER_TO, &buf);
	  if (status == 0)
	    {
	      char *s = rfc2047_decode_wrapper (buf, strlen (buf));
	      printf ("%s\t", s);
	      free (s);
	    }
	}
      free (buf);

      status = header_aget_value_unfold (hdr, MU_HEADER_SUBJECT, &buf);
      if (status == 0)
	{
	  char *s = rfc2047_decode_wrapper (buf, strlen (buf));
	  printf ("%s\n", s);
	  free (s);
	  free (buf);
	}
    }

  status = mailbox_close (mbox);
  if (status != 0)
    {
      mu_error (_("Could not close <%s>: %s."),
		mailbox_name, mu_strerror (status));
    }

  mailbox_destroy (&mbox);
  return 0;
}
