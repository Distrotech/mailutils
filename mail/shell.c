/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001, 2005, 2007, 2009-2012 Free Software
   Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>. */

#include "mail.h"

static void
expand_bang (char **pbuf, const char *arg, const char *last)
{
  char *tmp, *q;
  const char *p;
  size_t count = 0;
  
  for (p = arg; *p; p++)
    if (*p == '!')
      count++;

  if (count == 0)
    {
      *pbuf = mu_strdup (arg);
      return;
    }

  if (!last)
    {
      mu_error (_("No previous command"));
      return;
    }

  tmp = mu_alloc (strlen (arg) + count * (strlen (last) - 1) + 1);
  for (p = arg, q = tmp; *p; )
    {
      if (*p == '!')
	{
	  strcpy (q, last);
	  q += strlen (q);
	  p++;
	}
      else
	*q++ = *p++;
    }
  *q = 0;
  
  free (*pbuf);
  *pbuf = tmp;
}

int
mail_execute (int shell, char *progname, int argc, char **argv)
{
  int xargc, i, status, rc;
  char **xargv;
  char *buf;
  int expanded; /* Indicates whether argv has been bang-expanded */
  
  if (argc == 0)
    {
      /* No arguments mean execute a copy of the user shell */
      shell = 1;
    }
  
  xargc = argc;
  if (shell && argc < 3)
    xargc = 3;
  xargv = mu_calloc (xargc + 1, sizeof (xargv[0]));
  
  /* Expand arguments if required */
  if (mailvar_get (NULL, "bang", mailvar_type_boolean, 0) == 0)
    {
      int i;
      char *last = NULL;
      
      mailvar_get (&last, "gnu-last-command", mailvar_type_string, 0);
      expand_bang (xargv, progname, last);
      for (i = 1; i < argc; i++)
	expand_bang (xargv + i, argv[i], last);
      expanded = 1;
    }
  else
    {
      if (argc)
	xargv[0] = progname;
      for (i = 1; i < argc; i++)
	xargv[i] = argv[i];
      expanded = 0;
    }
  
  /* Reconstruct the command line and save it to gnu-last-command variable.
     Important: use argc (not xargc)!
  */
  mu_argcv_string (argc, xargv, &buf);
  mailvar_set ("gnu-last-command", buf, mailvar_type_string, 
	       MOPTF_QUIET | MOPTF_OVERWRITE);

  if (shell)
    {
      if (expanded)
	{
	  for (i = 0; i < argc; i++)
	    free (xargv[i]);
	  expanded = 0;
	}
      
      xargv[0] = getenv ("SHELL");
      if (argc == 0)
	{
	  xargv[1] = NULL;
	  xargc = 1;
	}
      else
	{
	  xargv[1] = "-c";
	  xargv[2] = buf;
	  xargv[3] = NULL;
	  xargc = 3;
	}
    }  
  
  rc = mu_spawnvp (xargv[0], xargv, &status);
  if (rc)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_spawnvp", xargv[0], rc);
    }
  /* FIXME:
  else if (status)
    mu_diag_output (MU_DIAG_NOTICE, ....
  */
  free (buf);
  if (expanded)
    for (i = 0; i < argc; i++)
      free (xargv[i]);
  free (xargv);
  return rc;
}

/*
 * sh[ell] [command] -- GNU extension
 * ![command] -- GNU extension
 */

int
mail_shell (int argc, char **argv)
{
  if (argv[0][0] == '!' && strlen (argv[0]) > 1)
    return mail_execute (1, argv[0] + 1, argc, argv);
  else if (argc > 1)
    return mail_execute (0, argv[1], argc-1, argv+1);
  else
    return mail_execute (1, NULL, 0, NULL);
  return 1;
}


