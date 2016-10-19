/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2005, 2007-2012, 2014-2016 Free Software
   Foundation, Inc.

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

#include <stdio.h>

#include <mailutils/mailutils.h>
#include <mailutils/cli.h>

static int messages_count (const char *);

/* are we loud or quiet? */
static int silent = 0;

static struct mu_option messages_options[] = {
  { "quiet",	'q',	NULL,	MU_OPTION_DEFAULT,
    N_("only display number of messages"),
    mu_c_bool, &silent },
  {"silent",	's',	NULL,	MU_OPTION_ALIAS },
  MU_OPTION_END
};

static char *capa[] = {
  "debug",
  "mailbox",
  "locking",
  NULL
};

static struct mu_option *options[] = { messages_options, NULL };

struct mu_cli_setup cli = {
  options,
  NULL,
  N_("GNU messages -- count the number of messages in a mailbox"),
  N_("[mailbox...]")
};

int
main (int argc, char **argv)
{
  int err = 0;

  /* Native Language Support */
  MU_APP_INIT_NLS ();

  /* register the formats.  */
  mu_register_all_mbox_formats ();

  mu_auth_register_module (&mu_auth_tls_module);
  mu_cli (argc, argv, &cli, capa, NULL, &argc, &argv);

  if (argc == 0 && messages_count (NULL) < 0)
    err = 1;
  else if (argc >= 1)
    {
      size_t i;
      
      for (i = 0; i < argc; i++)
	{
	  if (messages_count (argv[i]) < 0)
	    err = 1;
	}
    }

  return err;
}

static int
messages_count (const char *box)
{
  mu_mailbox_t mbox;
  mu_url_t url = NULL;
  size_t count;
  int status = 0;

  status =  mu_mailbox_create_default (&mbox, box);
  if (status != 0)
    {
      if (box)
	mu_error (_("could not create mailbox `%s': %s"),
		  box, mu_strerror (status));
      else
	mu_error (_("could not create default mailbox: %s"),
		  mu_strerror (status));
      return -1;
    }

  mu_mailbox_get_url (mbox, &url);
  box = mu_url_to_string (url);

  status =  mu_mailbox_open (mbox, MU_STREAM_READ);
  if (status != 0)
    {
      mu_error (_("could not open mailbox `%s': %s"),
		box, mu_strerror (status));
      return -1;
    }

  status = mu_mailbox_messages_count (mbox, &count);
  if (status != 0)
    {
      mu_error (_("could not count messages in mailbox `%s': %s"),
		box, mu_strerror (status));
      return -1;
    }

  if (silent)
    mu_printf ("%lu\n", (unsigned long) count);
  else
    mu_printf (_("Number of messages in %s: %lu\n"), box,
	       (unsigned long) count);

  status = mu_mailbox_close (mbox);
  if (status != 0)
    {
      mu_error (_("could not close `%s': %s"),
		box, mu_strerror (status));
      return -1;
    }

  mu_mailbox_destroy (&mbox);
  return count;
}
