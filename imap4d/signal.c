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


#define __USE_MISC
#include "imap4d.h"

RETSIGTYPE
imap4d_sigchld (int signo)
{
  pid_t pid;
  int status;

  while ((pid = waitpid (-1, &status, WNOHANG)) > 0)
    --children;
#ifndef HAVE_SIGACTION
  /* On some system, signal implements the unreliable semantic and
     has to be rearm.  */
  signal (signo, imap4d_sigchld);
#endif
}

/* Default signal handler to call the imap4d_bye() function */

RETSIGTYPE
imap4d_signal (int signo)
{
  syslog (LOG_CRIT, _("got signal %s"), strsignal (signo));
  /* Master process.  */
  if (util_is_master ())
    {
      syslog (LOG_CRIT, _("MASTER: exiting on signal"));
      exit (1);			/* abort(); */
    }

  switch (signo)
    {
    case SIGALRM:
      imap4d_bye (ERR_TIMEOUT);

    case SIGPIPE:
      exit (0);

    default:
      imap4d_bye (ERR_SIGNAL);
    }
}
