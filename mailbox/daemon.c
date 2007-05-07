/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2004, 2005 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library; if not, write to the
   Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301 USA */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

#include <mailutils/daemon.h>
#include <mu_umaxtostr.h>

static char *pidfile;
static pid_t current_pid;

int
mu_daemon_create_pidfile (const char *filename)
{
  const char *pid_string; 
  int fd;

  if (filename[0] != '/')
    {
      return 1; /* failure */
    }

  if (pidfile)
    free (pidfile);
  pidfile = strdup (filename);

  unlink (pidfile);
  current_pid = getpid ();
  
  if ((fd = open (pidfile, O_WRONLY | O_CREAT
		  | O_TRUNC | O_EXCL, 0644)) == -1)
    {
      return 2; /* failure */
    }
  
  pid_string = mu_umaxtostr (0, current_pid);
  write (fd, pid_string, strlen (pid_string));
  close (fd);
  
  atexit (mu_daemon_remove_pidfile);
  return 0;
}

void
mu_daemon_remove_pidfile (void)
{
  if (getpid () == current_pid)
    {
      unlink (pidfile);
      free (pidfile);
      pidfile = NULL;
    }
}

