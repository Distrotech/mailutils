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


#include "pop3d.h"

RETSIGTYPE
pop3d_sigchld (int signo)
{
  pid_t pid;
  int status;
  (void)signo;

  while ( (pid = waitpid(-1, &status, WNOHANG)) > 0)
      --children;
#ifndef HAVE_SIGACTION
  /* On some system, signal implements the unreliable semantic and
     has to be rearm.  */
  signal (signo, pop3d_sigchld);
#endif
}

/* Default signal handler to call the pop3d_abquit() function */

RETSIGTYPE
pop3d_signal (int signo)
{
  syslog (LOG_CRIT, _("got signal %s"), strsignal (signo));

  /* Master process.  */
  if (pop3d_is_master ())
    {
       syslog (LOG_CRIT, _("MASTER: exiting on signal"));
       exit (EXIT_FAILURE); 
    }

  if (signo == SIGALRM)
    pop3d_abquit (ERR_TIMEOUT);
  pop3d_abquit (ERR_SIGNAL);
}
