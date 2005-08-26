/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002, 2003,
   2004, 2005 Free Software Foundation, Inc.

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
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
   MA 02110-1301 USA */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#ifdef HAVE_MALLOC_H
# include <malloc.h>
#endif

#include <mailutils/mailutils.h>

static int messages_count (const char *);

const char *program_version = "messages (" PACKAGE_STRING ")";
static char doc[] = N_("GNU messages -- count the number of messages in a mailbox");
static char args_doc[] = N_("[mailbox...]");

static struct argp_option options[] = {
  { NULL,         0, NULL,  0,
    /* TRANSLATORS: 'messages' is a program name. Do not translate it! */
    N_("messages specific switches:"), 0},
  {"quiet",	'q',	0,	0,	N_("Only display number of messages")},
  {"silent",	's',	0,	0,	N_("Same as -q")},
  { 0 }
};

static const char *argp_capa[] = {
  "common",
  "license",
  "mailbox",
#ifdef WITH_TLS
  "tls",
#endif
  NULL
};

struct arguments
{
  int argc;
  char **argv;
};

/* are we loud or quiet? */
static int silent = 0;

static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  struct arguments *args = state->input;
  switch (key)
    {
    case 'q':
    case 's':
      silent = 1;
      break;
    case ARGP_KEY_ARG:
      args->argv = realloc (args->argv,
			    sizeof (char *) * (state->arg_num + 2));
      args->argv[state->arg_num] = arg;
      args->argv[state->arg_num + 1] = NULL;
      args->argc++;
      break;
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

static struct argp argp = {
  options,
  parse_opt,
  args_doc,
  doc,
  NULL,
  NULL, NULL
};

int
main (int argc, char **argv)
{
  int i = 1;
  int err = 0;
  struct arguments args = {0, NULL};

  /* Native Language Support */
  mu_init_nls ();

  mu_argp_init (program_version, NULL);
#ifdef WITH_TLS
  mu_tls_init_client_argp ();
#endif
  mu_argp_parse (&argp, &argc, &argv, 0, argp_capa, NULL, &args);

  /* register the formats.  */
  mu_register_all_mbox_formats ();

  if (args.argc < 1 && messages_count (NULL) < 0)
    err = 1;
  else if (args.argc >= 1)
    {
      for (i = 0; i < args.argc; i++)
	{
	  if (messages_count (args.argv[i]) < 0)
	    err = 1;
	}
    }

  return err;
}

static int
messages_count (const char *box)
{
  mailbox_t mbox;
  url_t url = NULL;
  size_t count;
  int status = 0;

  status =  mu_mailbox_create_default (&mbox, box);
  if (status != 0)
    {
      if (box)
	mu_error (_("Could not create mailbox `%s': %s"),
		  box, mu_strerror (status));
      else
	mu_error (_("Could not create default mailbox: %s"),
		  mu_strerror (status));
      return -1;
    }

  mu_mailbox_get_url (mbox, &url);
  box = url_to_string (url);

  status =  mu_mailbox_open (mbox, MU_STREAM_READ);
  if (status != 0)
    {
      mu_error (_("Could not open mailbox `%s': %s"),
		box, mu_strerror (status));
      return -1;
    }

  status = mu_mailbox_messages_count (mbox, &count);
  if (status != 0)
    {
      mu_error (_("Could not count messages in mailbox `%s': %s"),
		box, mu_strerror (status));
      return -1;
    }

  if (silent)
    printf ("%d\n", count);
  else
    printf (_("Number of messages in %s: %d\n"), box, count);

  status = mu_mailbox_close (mbox);
  if (status != 0)
    {
      mu_error (_("Could not close `%s': %s"),
		box, mu_strerror (status));
      return -1;
    }

  mu_mailbox_destroy (&mbox);
  return count;
}
