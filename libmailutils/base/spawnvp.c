/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2003, 2005-2007, 2009-2012 Free Software
   Foundation, Inc.

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

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

/* See Advanced Programming in the UNIX Environment, Stevens,
 * program  10.20 for the rational for the signal handling. I
 * had to look it up, so if somebody else is curious, thats where
 * to find it.
 */
int 
mu_spawnvp (const char *prog, char *av[], int *stat)
{
  pid_t pid;
  int err = 0;
  int progstat;
  struct sigaction ignore;
  struct sigaction saveintr;
  struct sigaction savequit;
  sigset_t chldmask;
  sigset_t savemask;

  if (!prog || !av)
    return EINVAL;

  ignore.sa_handler = SIG_IGN;	/* ignore SIGINT and SIGQUIT */
  ignore.sa_flags = 0;
  sigemptyset (&ignore.sa_mask);

  if (sigaction (SIGINT, &ignore, &saveintr) < 0)
    return errno;
  if (sigaction (SIGQUIT, &ignore, &savequit) < 0)
    {
      sigaction (SIGINT, &saveintr, NULL);
      return errno;
    }

  sigemptyset (&chldmask);	/* now block SIGCHLD */
  sigaddset (&chldmask, SIGCHLD);

  if (sigprocmask (SIG_BLOCK, &chldmask, &savemask) < 0)
    {
      sigaction (SIGINT, &saveintr, NULL);
      sigaction (SIGQUIT, &savequit, NULL);
      return errno;
    }

  pid = fork ();

  if (pid < 0)
    {
      err = errno;
    }
  else if (pid == 0)
    {				/* child */
      /* restore previous signal actions & reset signal mask */
      sigaction (SIGINT, &saveintr, NULL);
      sigaction (SIGQUIT, &savequit, NULL);
      sigprocmask (SIG_SETMASK, &savemask, NULL);

      execvp (prog, av);
      _exit (127);		/* exec error */
    }
  else
    {				/* parent */
      while (waitpid (pid, &progstat, 0) < 0)
	if (errno != EINTR)
	  {
	    err = errno;	/* error other than EINTR from waitpid() */
	    break;
	  }
      if (err == 0 && stat)
	*stat = progstat;
    }

  /* restore previous signal actions & reset signal mask */
  /* preserve first error number, but still try and reset the signals */
  if (sigaction (SIGINT, &saveintr, NULL) < 0)
    err = err ? err : errno;
  if (sigaction (SIGQUIT, &savequit, NULL) < 0)
    err = err ? err : errno;
  if (sigprocmask (SIG_SETMASK, &savemask, NULL) < 0)
    err = err ? err : errno;

  return err;
}

