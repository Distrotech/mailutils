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
 * se[t] [name[=[string]] ...] [name=number ...] [noname ...]
 */

int
mail_set (int argc, char **argv)
{
  if (argc < 2)
    {
      /* step through the environment */
    }
  else
    {
      int i = 0;
      char *var = NULL, *value = NULL;
      for (i = 1; i < argc; i++)
	{
	  if (!strncmp ("no", argv[i], 2))
	    {
	      /* unset variable */
	    }
	  else if (strchr (argv[i], '=') != NULL)
	    {
	      int j = 0;
	      var = strdup (argv[i]);
	      for (j = 0; j < strlen (var); j++)
		if (var[j] == '=')
		  {
		    var[j] = '\0';
		    break;
		  }
	      value = strdup (&var[j+1]);
	      /* set var = value */
	      free (var);
	      free (value);
	    }
	  else
	    {
	      /* set var = NULL */
	    }
	}
      return 0;
    }
  return 1;
}
