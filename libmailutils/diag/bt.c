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

#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <mailutils/diag.h>
#include <mailutils/util.h>

/* Dump a stack trace and terminate current program.

   The trace is written to file /tmp/mailutils.PROG.PID, where PROG
   is the file name of the program, and PID its PID. */
   
void
mu_gdb_bt ()
{
  int i;
  pid_t master_pid = getpid ();
  pid_t pid;
  static char buf[1024];
  static char fname[1024];
  int fd;
  char *argv[8];
  
  if (!mu_program_name)
    abort ();
  sprintf (fname, "/tmp/mailutils.%s.%lu",
	   mu_program_name, (unsigned long) master_pid);
  
  pid = fork ();
  if (pid == (pid_t)-1)
    abort ();
  if (pid)
    {
      sleep (10);
      abort ();
    }

  for (i = mu_getmaxfd (); i >= 0; i--)
    close (i);

  fd = open (fname, O_WRONLY|O_CREAT, 0600);
  if (fd == -1)
    abort ();

  dup2 (fd, 1);
  dup2 (fd, 2);
  close (fd);
  
  argv[0] = "/usr/bin/gdb";
  argv[1] = (char*) mu_full_program_name;
  sprintf (buf, "%lu", (unsigned long) master_pid);
  argv[2] = buf;
  argv[3] = "-ex";
  argv[4] = "bt";
  argv[5] = "-ex";
  argv[6] = "kill";
  argv[7] = NULL;

  execvp (argv[0], argv);
  abort ();
}

