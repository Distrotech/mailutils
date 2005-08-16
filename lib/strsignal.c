/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2005  Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA  */

#if HAVE_CONFIG_H
# include <config.h>
#endif
#include <unistd.h>
#include <signal.h>
#ifndef SYS_SIGLIST_DECLARED
/* For snprintf ().  */
#include <stdio.h>
#endif

#ifndef __P
# ifdef __STDC__
#  define args args
# else
#  define args) (
# endif
#endif /*__P */

/* Some systems do not define NSIG in <signal.h>.  */
#ifndef NSIG
# ifdef  _NSIG
#  define NSIG    _NSIG
# else
#  define NSIG    32
# endif
#endif

/* FIXME: Should probably go in a .h somewhere.  */
char *strsignal (int);

char *
strsignal (int signo)
{
#ifdef SYS_SIGLIST_DECLARED
  /* Let's try to protect ourself a little.  */
  if (signo > 0 || signo < NSIG)
    return (char *)sys_siglist[signo];
  return (char *)"";
#else
  static char buf[64];
  snprintf (buf, sizeof buf, "Signal %d", signo);
  return buf;
#endif
}
