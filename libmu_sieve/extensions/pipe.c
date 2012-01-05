/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2007, 2009-2012 Free Software Foundation, Inc.

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
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <regex.h>
#include <mailutils/sieve.h>
#include <mailutils/prog.h>

#define ONERR(rc, diag, arg)						\
  if (rc)								\
    {									\
      error_diag = diag;						\
      error_arg = arg;							\
      break;								\
    }

#define PIPE_ENVELOPE 0x01
#define PIPE_HEADERS  0x02
#define PIPE_BODY     0x04
#define PIPE_ALL (PIPE_ENVELOPE | PIPE_HEADERS | PIPE_BODY)

int
sieve_pipe (mu_sieve_machine_t mach, mu_list_t args, mu_list_t tags, int test)
{
  int retval = 0;
  int rc, result;
  mu_message_t msg;
  mu_sieve_value_t *val;
  char *cmd;
  mu_stream_t pstr;
  mu_envelope_t env;
  const char *error_diag = NULL;
  const char *error_arg = NULL;
  int pipe_mask = 0;
  
  val = mu_sieve_value_get (args, 0);
  if (!val)
    {
      mu_sieve_error (mach, "%lu: %s",
		      (unsigned long) mu_sieve_get_message_num (mach),
		      _("cannot get command!"));
      mu_sieve_abort (mach);
    }
  cmd = val->v.string;

  if (mu_sieve_get_debug_level (mach) & MU_SIEVE_DEBUG_TRACE)
    {
      mu_sieve_debug (mach, test ? "PIPE (test)" : "PIPE (action)");
    }

  if (mu_sieve_is_dry_run (mach))
    return 0;

  msg = mu_sieve_get_message (mach);
  mu_message_get_envelope (msg, &env);

  if (mu_sieve_tag_lookup (tags, "envelope", NULL))
    pipe_mask |= PIPE_ENVELOPE;
  if (mu_sieve_tag_lookup (tags, "header", NULL))
    pipe_mask |= PIPE_HEADERS;
  if (mu_sieve_tag_lookup (tags, "body", NULL))
    pipe_mask |= PIPE_BODY;
  if (pipe_mask == 0)
    pipe_mask = PIPE_ALL;
  do
    {
      mu_stream_t mstr = NULL;
      
      rc = mu_command_stream_create (&pstr, cmd, MU_STREAM_WRITE);
      ONERR (rc, _("cannot create command stream"), cmd);

      if (pipe_mask & PIPE_ENVELOPE)
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

      if (pipe_mask & PIPE_HEADERS)
	{
	  mu_header_t hdr;
	  
	  mu_message_get_header (msg, &hdr);
	  mu_header_get_streamref (hdr, &mstr);
	  rc = mu_stream_copy (pstr, mstr, 0, NULL);
	  ONERR (rc, _("copying headers failed"), cmd);
	  mu_stream_destroy (&mstr);
	}
      if (pipe_mask & PIPE_BODY)
	{
	  mu_body_t body;

	  mu_message_get_body (msg, &body);
	  mu_body_get_streamref (body, &mstr);
	  rc = mu_stream_copy (pstr, mstr, 0, NULL);
	  ONERR (rc, _("copying body failed"), cmd);
	  mu_stream_destroy (&mstr);
	}
    }
  while (0);

  result = mu_stream_close (pstr);

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
      mu_stream_destroy (&pstr);
      mu_sieve_abort (mach);
    }

  if (test)
    {
      int code = 0;
      int status;
      
      rc = mu_stream_ioctl (pstr, MU_IOCTL_PROGSTREAM,
			    MU_IOCTL_PROG_STATUS, &status);
      if (rc)
	{
	  mu_stream_destroy (&pstr);
	  mu_sieve_abort (mach);
	}

      if (mu_sieve_tag_lookup (tags, "exit", &val))
	code = val->v.number;
      if (result == 0)
	retval = code == 0;
      else if (result == MU_ERR_PROCESS_EXITED)
	retval = code == WEXITSTATUS (status);
      else if (result == MU_ERR_PROCESS_SIGNALED)
	{
	  int signo = WTERMSIG (status);
	  if (mu_sieve_tag_lookup (tags, "signal", &val))
	    retval = signo == val->v.number;
	  else
	    {
	      mu_stream_destroy (&pstr);
	      mu_sieve_abort (mach);
	    }
	}
      else
	{
	  mu_sieve_error (mach, "%lu: %s",
			  (unsigned long) mu_sieve_get_message_num (mach),
			  mu_strerror (result));
	  mu_stream_destroy (&pstr);
	  mu_sieve_abort (mach);
	}
    }
	    
  mu_stream_destroy (&pstr);

  return retval;
}

int
sieve_action_pipe (mu_sieve_machine_t mach, mu_list_t args, mu_list_t tags)
{
  mu_sieve_log_action (mach, "PIPE", NULL);
  return sieve_pipe (mach, args, tags, 0);
}

int
sieve_test_pipe (mu_sieve_machine_t mach, mu_list_t args, mu_list_t tags)
{
  return sieve_pipe (mach, args, tags, 1);
}


/* Tagged arguments: */
static mu_sieve_tag_def_t pipe_tags[] = {
  { "envelope", SVT_VOID },
  { "body", SVT_VOID },
  { "header", SVT_VOID },
  { NULL }
};

static mu_sieve_tag_group_t pipe_action_tag_groups[] = {
  { pipe_tags, NULL },
  { NULL }
};

static mu_sieve_tag_def_t pipe_test_tags[] = {
  { "exit", SVT_NUMBER },
  { "signal", SVT_NUMBER },
  { NULL }
};

static mu_sieve_tag_group_t pipe_test_tag_groups[] = {
  { pipe_tags, NULL },
  { pipe_test_tags, NULL },
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
  int rc;
  rc = mu_sieve_register_action (mach, "pipe", sieve_action_pipe,
				 pipe_args, pipe_action_tag_groups, 1);
  if (rc)
    return rc;
  return mu_sieve_register_test (mach, "pipe", sieve_test_pipe,
				 pipe_args, pipe_test_tag_groups, 1);
}
