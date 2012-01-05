/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2011-2012 Free Software Foundation, Inc.

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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <unistd.h>
#include <mailutils/diag.h>

/* A debugging hook.

   Suspend execution to let the developer to attach to the current process
   using gdb.

   Wait at most TO seconds.  If TO is 0, wait forever. */

void
mu_wd (unsigned to)
{
  unsigned volatile _count_down;
  pid_t pid = getpid ();

  if (to)
    mu_diag_output (MU_DIAG_CRIT,
		    "process %lu is waiting for debug (%u seconds left)",
		    (unsigned long) pid, to);
  else
    mu_diag_output (MU_DIAG_CRIT,
		    "process %lu is waiting for debug",
		    (unsigned long) pid);
  mu_diag_output (MU_DIAG_CRIT,
		  "to attach: gdb -ex 'set variable mu_wd::_count_down=0' %s %lu",
		  mu_full_program_name, (unsigned long) pid);
  if (to)
    {
      _count_down = to;
      while (_count_down--);
	{
	  sleep (1);
	}
    }
  else
    {
      _count_down = 1;
      while (_count_down)
	{
	  sleep (1);
	}
    }
  mu_diag_output (MU_DIAG_CRIT, "process %lu finished waiting",
		  (unsigned long) pid);
}
