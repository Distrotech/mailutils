/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2007, 2009, 2010 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library.  If not, see
   <http://www.gnu.org/licenses/>. */

/* Syntax: pipe [:envelope] <program: string>

   The pipe action executes a shell command specified by its
   argument and pipes the entire message to its standard input.
   The envelope of the message is included, if the :envelope tag is given.

   Notes/FIXME: 1. it would be nice to implement meta-variables in
		<program call> which would expand to various
		items from the message being handled.
		2. :mime tag could be useful too.
*/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <signal.h>
#include <regex.h>
#include <mailutils/sieve.h>

int
sieve_action_pipe (mu_sieve_machine_t mach, mu_list_t args, mu_list_t tags)
{
  int rc;
  mu_message_t msg;
  mu_sieve_value_t *val;
  char *cmd;
  mu_stream_t mstr, pstr;
  mu_envelope_t env;
  const char *error_diag = NULL;
  const char *error_arg = NULL;
#define ONERR(rc, diag, arg)						\
  if (rc)								\
    {									\
      error_diag = diag;						\
      error_arg = arg;							\
      break;								\
    }

  val = mu_sieve_value_get (args, 0);
  if (!val)
    {
      mu_sieve_error (mach, "%lu: %s",
		      (unsigned long) mu_sieve_get_message_num (mach),
		      _("cannot get command!"));
      mu_sieve_abort (mach);
    }
  cmd = val->v.string;

  mu_sieve_log_action (mach, "PIPE", NULL);
  if (mu_sieve_get_debug_level (mach) & MU_SIEVE_DEBUG_TRACE)
    {
      mu_sieve_debug (mach, "PIPE");
    }

  if (mu_sieve_is_dry_run (mach))
    return 0;

  msg = mu_sieve_get_message (mach);
  mu_message_get_envelope (msg, &env);

  do
    {
      rc = mu_message_get_streamref (msg, &mstr);
      ONERR (rc, _("cannot get message stream"), NULL);

      rc = mu_prog_stream_create (&pstr, cmd, MU_STREAM_WRITE);
      ONERR (rc, _("cannot create command stream"), cmd);

      if (mu_sieve_tag_lookup (tags, "envelope", &val))
	{
	  char *p;

	  rc = mu_envelope_aget_sender (env, &p);
	  ONERR (rc, _("cannot get envelope sender"), NULL);
	  rc = mu_stream_write (pstr, "From ", 5, NULL);
	  ONERR (rc, _("stream write failed"), NULL);
	  mu_stream_write (pstr, p, strlen (p), NULL);
	  free (p);
	  rc = mu_stream_write (pstr, " ", 1, NULL);
	  ONERR (rc, _("stream write failed"), NULL);
	  rc = mu_envelope_aget_date (env, &p);
	  ONERR (rc, _("cannot get envelope date"), NULL);
	  rc = mu_stream_write (pstr, p, strlen (p), NULL);
	  ONERR (rc, _("stream write failed"), NULL);
	  free (p);
	  rc = mu_stream_write (pstr, "\n", 1, NULL);
	  ONERR (rc, _("stream write failed"), NULL);
	}

      rc = mu_stream_copy (pstr, mstr, 0, NULL);
      ONERR (rc, _("command failed"), cmd);
    }
  while (0);

  mu_stream_destroy (&mstr);
  mu_stream_close (pstr);
  mu_stream_destroy (&pstr);

  if (rc)
    {
      if (error_arg)
	mu_sieve_error (mach, "%lu: %s: %s: %s",
			(unsigned long) mu_sieve_get_message_num (mach),
			error_diag,
			error_arg,
			mu_strerror (rc));
      else
	mu_sieve_error (mach, "%lu: %s: %s",
			(unsigned long) mu_sieve_get_message_num (mach),
			error_diag,
			mu_strerror (rc));
      mu_sieve_abort (mach);
    }

  return 0;
}

/* Tagged arguments: */
static mu_sieve_tag_def_t pipe_tags[] = {
  { "envelope", SVT_VOID },
  { NULL }
};

static mu_sieve_tag_group_t pipe_tag_groups[] = {
  { pipe_tags, NULL },
  { NULL }
};

/* Required arguments: */
static mu_sieve_data_type pipe_args[] = {
  SVT_STRING,			/* program call */
  SVT_VOID
};

int
SIEVE_EXPORT (pipe, init) (mu_sieve_machine_t mach)
{
  return mu_sieve_register_action (mach, "pipe", sieve_action_pipe,
				   pipe_args, pipe_tag_groups, 1);
}
