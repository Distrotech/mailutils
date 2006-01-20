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
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
   MA 02110-1301 USA */

#include "mail.h"

/*
 * se[t] [name[=[string]] ...] [name=number ...] [noname ...]
 */

/*
 * NOTE: ask is a synonym for asksub
 */

int
mail_set (int argc, char **argv)
{
  if (argc < 2)
    {
      util_printenv (1);
      return 0;
    }
  else
    {
      int i = 0;

      for (i = 1; i < argc; i++)
	{
	  if (!strncmp ("no", argv[i], 2))
	    {
	      util_setenv (&argv[i][2], NULL, Mail_env_boolean, 1);
	    }
	  else if (i+1 < argc && argv[i+1][0] == '=')
	    {
	      int nval;
	      char *name = argv[i];
	      char *p;
	      
	      i += 2;
	      if (i >= argc)
		break;

	      nval = strtoul (argv[i], &p, 0);
	      if (*p == 0)
		util_setenv (name, &nval, Mail_env_number, 1);
	      else
		util_setenv (name, argv[i], Mail_env_string, 1);
	    }
	  else
	    {
	      int dummy;
	      util_setenv (argv[i], &dummy, Mail_env_boolean, 1);
	    }
	}
      return 0;
    }
  return 1;
}
