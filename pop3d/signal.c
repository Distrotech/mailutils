/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002, 2007 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
   MA 02110-1301 USA */

#include "pop3d.h"

static int need_cleanup = 0;

void
process_cleanup ()
{
  pid_t pid;
  int status;
  
  if (need_cleanup)
    {
      need_cleanup = 0;
      while ( (pid = waitpid (-1, &status, WNOHANG)) > 0)
	--children;
    }
}

RETSIGTYPE
pop3d_sigchld (int signo ARG_UNUSED)
{
  need_cleanup = 1;
#ifndef HAVE_SIGACTION
  signal (signo, pop3d_sigchld);
#endif
}

/* Default signal handler to call the pop3d_abquit() function */

RETSIGTYPE
pop3d_signal (int signo)
{
  int code;
  
  syslog (LOG_CRIT, _("Got signal %s"), strsignal (signo));

  /* Master process.  */
  if (pop3d_is_master ())
    {
       syslog (LOG_CRIT, _("MASTER: exiting on signal"));
       exit (EXIT_FAILURE); 
    }

  switch (signo)
    {
    case SIGALRM:
      code = ERR_TIMEOUT;
      break;
      
    case SIGPIPE:
      code = ERR_NO_OFILE;
      break;
      
    default:
      code = ERR_SIGNAL;
    }
  pop3d_abquit (code);
}
