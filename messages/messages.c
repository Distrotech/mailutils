/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Library Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#include "config.h"

#include <mailutils/mailutils.h>

#include <stdio.h>

#ifdef HAVE_MALLOC_H
# include <malloc.h>
#endif

#include <mu_argp.h>

static int messages_count (const char *);

const char *argp_program_version = "messages (" PACKAGE ") " VERSION;
const char *argp_program_bug_address = "<bug-mailutils@gnu.org>";
static char doc[] = "GNU messages -- count the number of messages in a mailbox";
static char args_doc[] = "[mailbox...]";

static struct argp_option options[] = {
  {NULL, 0, NULL, 0,
   "messages specific switches:", 0},
  {"quiet",	'q',	0,	0,	"Only display number of messages"},
  {"silent",	's',	0,	0,	"Same as -q"},
  { 0 }
};

static const char *argp_capa[] = {
  "common",
  "license",
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
  list_t bookie;
  int err = 0;
  struct arguments args = {0, NULL};

  mu_argp_parse (&argp, &argc, &argv, 0, argp_capa, NULL, &args);

  registrar_get_list (&bookie);
  list_append (bookie, path_record);
  list_append (bookie, imap_record);
  list_append (bookie, pop_record);

  if (args.argc < 1 && messages_count (NULL) < 0)
      err = 1;
  else if (args.argc >= 1)
    {
      for (i=0; i < args.argc; i++)
	if (messages_count (args.argv[i]) < 0)
	  err = 1;
    }

  return err;
}

static int
messages_count (const char *box)
{
  mailbox_t mbox;
  url_t url = NULL;
  size_t count;

  if (mailbox_create_default (&mbox, box) != 0)
    {
      fprintf (stderr, "Couldn't create mailbox %s.\n", (box) ? box : "");
      return -1;
    }

  mailbox_get_url (mbox, &url);
  box = url_to_string (url);

  if (mailbox_open (mbox, MU_STREAM_READ) != 0)
    {
      fprintf (stderr, "Couldn't open mailbox %s.\n", box);
      return -1;
    }

  if (mailbox_messages_count (mbox, &count) != 0)
    {
      fprintf (stderr, "Couldn't count messages in %s.\n", box);
      return -1;
    }

  if (silent)
    printf ("%d\n", count);
  else
    printf ("Number of messages in %s: %d\n", box, count);

  if (mailbox_close (mbox) != 0)
    {
      fprintf (stderr, "Couldn't close %s.\n", box);
      return -1;
    }
  mailbox_destroy (&mbox);
  return count;
}
