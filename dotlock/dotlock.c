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

#include <stdlib.h>

#include <argp.h>

#include <mailutils/errno.h>
#include <mailutils/locker.h>

#define MU_DL_EX_OK      0
#define MU_DL_EX_ERROR   1
#define MU_DL_EX_EXIST   3

const char *argp_program_version = "GNU dotlock (" PACKAGE ") " VERSION;
const char *argp_program_bug_address = "<bug-mailutils@gnu.org>";
static char doc[] =
  "GNU dotlock -- lock mail spool files"
  "\v"
  "Returns 0 on success, 3 if the locking the file fails because"
  " it's already locked, and 1 if some other kind of error occurred."
  ;
static char args_doc[] = "FILE";
error_t argp_err_exit_status = MU_DL_EX_ERROR;

static struct argp_option options[] = {
  {"unlock", 'u', NULL, 0,
   "Unlock", 0},

  {"force", 'f', "MINUTES", OPTION_ARG_OPTIONAL,
   "Forcibly break an existing lock older than a certain time", 0},

  {"retry", 'r', "RETRIES", OPTION_ARG_OPTIONAL,
   "Retry the lock a few times", 0},

  {"debug", 'd', NULL, 0,
   "Print details of failure reasons to stderr", 0},

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

    case 'r':
      if (arg)
	{
	  retries = atoi (arg);
	  if (retries == 0)
	    argp_error (state, "RETRIES must be greater than 0");
	}
      flags |= MU_LOCKER_RETRY;
      break;

    case 'f':
      if (arg)
	{
	  force = atoi (arg);
	  if (force == 0)
	    argp_error (state, "MINUTES must be greater than 0");
	  force *= 60;
	}
      flags |= MU_LOCKER_TIME;
      break;

    case ARGP_KEY_ARG:
      if (file)
	argp_error (state, "only one FILE can be specified");
      file = arg;
      break;

    case ARGP_KEY_NO_ARGS:
      argp_error (state, "FILE must be specified");

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

  argp_parse (&argp, argc, argv, 0, NULL, NULL);

  if ((err = locker_create (&locker, file, flags)))
    {
      if (debug)
	fprintf (stderr, "locker create failed: %s\n", mu_errstring (err));
      return MU_DL_EX_ERROR;
    }

  locker_set_flags (locker, flags);

  if (force != 0)
    locker_set_expire_time (locker, force);

  if (retries != 0)
    locker_set_retries (locker, retries);

  if (unlock)
    err = locker_remove_lock (locker);
  else
    err = locker_lock (locker);

  locker_destroy (&locker);

  if (debug && err)
    fprintf (stderr, "%slocking %s failed: %s\n",
	     unlock ? "un" : "", file, mu_errstring (err));

  switch (err)
    {
    case 0:
      err = MU_DL_EX_OK;
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
