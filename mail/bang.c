/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000 Free Software Foundation, Inc.

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

#include "mail.h"

/*
 * !command
 */

int
mail_bang (int argc, char **argv)
{
  int pid = fork ();
  if (pid == 0)
    {
      const char *shell = getenv ("SHELL");
      const char **argvec;
      char *buf = NULL;

      if (shell == NULL)
	shell = "/bin/sh";

      /* 1(shell) + 1 (-c) + 1(arg) + 1 (null) = 4  */
      argvec = malloc (4 * (sizeof (char *)));

      /* Nuke the !  */
      /* FIXME: should they do that upstairs ?  */
      if (argv[0][0] == '!')
	argv[0][0] = ' ';

      argcv_string (argc, argv, &buf);

      argvec[0] = shell;
      argvec[1] = "-c";
      argvec[2] = buf;
      argvec[3] = NULL;

      execv (shell, argvec);
      free (buf); /* Being cute, nuke it when finish testing.  */
      free (argvec);
      return 1;
    }
  else if (pid > 0)
    {
      while (waitpid(pid, NULL, 0) == -1)
	/* do nothing */;
      return 0;
    }
  return 1;
}
