/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999 Free Software Foundation, Inc.

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
 * sh[ell] [command] -- GNU extension
 * ![command] -- GNU extension
 */

int
mail_shell (int argc, char **argv)
{
  if (argv[0][0] == '!' && strlen (argv[0]) > 1)
    {
      char *buf = NULL;
      argv[0][0] = ' ';
      argcv_string (argc, argv, &buf);
      if (buf)
	{
	  int ret = util_do_command ("shell %s", &buf[1]);
	  free (buf);
	  return ret;
	}
      else
	return util_do_command ("shell");
    }
  else if (argc > 1)
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
	  
	  argcv_string (argc-1, &argv[1], &buf);
	  
	  argvec[0] = shell;
	  argvec[1] = "-c";
	  argvec[2] = buf;
	  argvec[3] = NULL;
	  
	  /* why does this complain if argvec[2] is in the path but not
	     fully qualified ? */
	  execvp (shell, argvec);
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
  else
    {
      char *shell = getenv ("SHELL");
      if (!shell)
	shell = strdup ("/bin/sh");
      return util_do_command ("shell %s", shell);
    }
  return 1;
}



