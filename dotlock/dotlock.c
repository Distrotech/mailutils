/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002 Free Software Foundation, Inc.

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
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#ifdef __EXT_QNX
# undef __EXT_QNX
#endif
#include <unistd.h>

#include <mailutils/argp.h>
#include <mailutils/errno.h>
#include <mailutils/locker.h>
#include <mailutils/nls.h>

const char *argp_program_version = "GNU dotlock (" PACKAGE_STRING ")";
static char doc[] =
N_("GNU dotlock -- lock mail spool files"
   "\v"
   "Returns 0 on success, 3 if the locking the file fails because"
   " it's already locked, and 1 if some other kind of error occurred.");

static char args_doc[] = N_("FILE");
error_t argp_err_exit_status = MU_DL_EX_ERROR;

static struct argp_option options[] = {
  {"unlock", 'u', NULL, 0,
   N_("Unlock"), 0},

  {"force", 'f', N_("MINUTES"), OPTION_ARG_OPTIONAL,
   N_("Forcibly break an existing lock older than a certain time"), 0},

  {"retry", 'r', N_("RETRIES"), OPTION_ARG_OPTIONAL,
   N_("Retry the lock a few times"), 0},

  {"debug", 'd', NULL, 0,
   N_("Print details of failure reasons to stderr"), 0},

  {"test", 'T', N_("PROGRAM"), OPTION_HIDDEN,
   N_("Test external dotlocker"), 0},

  {NULL, 0, NULL, 0, NULL, 0}
};

static error_t parse_opt (int key, char *arg, struct argp_state *state);

static struct argp argp = {
  options,
  parse_opt,
  args_doc,
  doc,
};

static const char *file;
static int unlock;
static int flags;
static int retries;
static int force;
static int debug;
static const char *program;

static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  switch (key)
    {
    case 'd':
      debug = 1;
      break;

    case 'u':
      unlock = 1;
      break;

    case 'T':
      /* This options exists only to test whether internal and external
	 locking work correctly/the same. */
      flags |= MU_LOCKER_EXTERNAL;
      program = arg;
      break;

    case 'r':
      if (arg)
	{
	  retries = atoi (arg);
	  if (retries <= 0)
	    argp_error (state, _("RETRIES must be greater than 0"));
	}
      flags |= MU_LOCKER_RETRY;
      break;

    case 'f':
      if (arg)
	{
	  force = atoi (arg);
	  if (force <= 0)
	    argp_error (state, _("MINUTES must be greater than 0"));
	  force *= 60;
	}
      flags |= MU_LOCKER_TIME;
      break;

    case ARGP_KEY_ARG:
      if (file)
	argp_error (state, _("only one FILE can be specified"));
      file = arg;
      break;

    case ARGP_KEY_NO_ARGS:
      argp_error (state, _("FILE must be specified"));

    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

int
main (int argc, char *argv[])
{
  locker_t locker = 0;
  int err = 0;
  pid_t usergid = getgid();
  pid_t mailgid = getegid();

  /* Native Language Support */
  mu_init_nls ();

  /* Drop permissions during argument parsing. */

  if(setegid(usergid) < 0)
    return MU_DL_EX_ERROR;

  argp_parse (&argp, argc, argv, 0, NULL, NULL);

  if ((err = locker_create (&locker, file, flags)))
    {
      if (debug)
	fprintf (stderr, _("locker create failed: %s\n"), mu_errstring (err));
      return MU_DL_EX_ERROR;
    }

  locker_set_flags (locker, flags);

  if (force != 0)
    locker_set_expire_time (locker, force);

  if (retries != 0)
    locker_set_retries (locker, retries);

  if (program != 0)
    locker_set_external (locker, program);

  if(setegid(mailgid) < 0)
    return MU_DL_EX_ERROR;

  if (unlock)
    err = locker_remove_lock (locker);
  else
    err = locker_lock (locker);

  setegid(usergid);

  locker_destroy (&locker);

  if (debug && err)
    fprintf (stderr, _("%s %s failed: %s\n"),
	     unlock ? _("unlocking") : _("locking"), file, mu_errstring (err));

  switch (err)
    {
    case 0:
      err = MU_DL_EX_OK;
      break;
    case EPERM:
      err = MU_DL_EX_PERM;
      break;
    case MU_ERR_LOCK_NOT_HELD:
      err = MU_DL_EX_NEXIST;
      break;
    case MU_ERR_LOCK_CONFLICT:
      err = MU_DL_EX_EXIST;
      break;
    default:
      err = MU_DL_EX_ERROR;
      break;
    }

  return err;
}

