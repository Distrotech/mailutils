/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001 Free Software Foundation, Inc.

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
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA  */

#include "mail.h"

int
mail_execute (int shell, int argc, char **argv)
{
  pid_t pid = fork ();

  if (pid == 0)
    {
      if (shell)
	{
	  if (argc == 0)
	    {
	      argv = xmalloc (sizeof (argv[0]) * 2);
	      argv[0] = getenv ("SHELL");
	      argv[1] = NULL;
	      argc = 1;
	    }
	  else
	    {
	      char *buf = NULL;

	      while (isspace (**argv))
		(*argv)++;
	      argcv_string (argc, &argv[0], &buf);

	      /* 1(shell) + 1 (-c) + 1(arg) + 1 (null) = 4  */
	      argv = xmalloc (4 * (sizeof (argv[0])));
	  
	      argv[0] = getenv ("SHELL");
	      argv[1] = "-c";
	      argv[2] = buf;
	      argv[3] = NULL;

	      argc = 3;
	    }
	}
      
      execvp (argv[0], argv);
      exit (1);
    }
  else if (pid > 0)
    {
      while (waitpid (pid, NULL, 0) == -1)
	/* do nothing */;
      return 0;
    }
  else if (pid < 0)
    {
      mu_error ("fork failed: %s", mu_strerror (errno));
      return 1;
    }
}

/*
 * sh[ell] [command] -- GNU extension
 * ![command] -- GNU extension
 */

int
mail_shell (int argc, char **argv)
{
  if (argv[0][0] == '!' && strlen (argv[0]) > 1)
    {
      argv[0][0] = ' ';
      return mail_execute (1, argc, argv);
    }
  else if (argc > 1)
    {
      return mail_execute (0, argc-1, argv+1);
    }
  else
    {
      return mail_execute (1, 0, NULL);
    }
  return 1;
}



