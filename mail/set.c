/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2001 Free Software Foundation, Inc.

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

/*
 * NOTE: ask is a synonym for asksub
 */

int
mail_set (int argc, char **argv)
{
  if (argc < 2)
    {
      return util_printenv (1);
    }
  else
    {
      int i = 0;
      char *var = NULL, *value = NULL;
      struct mail_env_entry *entry = NULL;
      for (i = 1; i < argc; i++)
	{
	  if (!strncmp ("no", argv[i], 2))
	    {
	      entry = util_find_env (&argv[i][2]);
	      if (entry == NULL)
		return 1;
	      entry->set = 0;
	      if (entry->value)
		free (entry->value);
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
	      entry = util_find_env (var);
	      free (var);
	      if (entry == NULL)
		return 1;
	      entry->set = 1;
	      if (entry->value)
		free (entry->value);
	      entry->value = value;
	    }
	  else
	    {
	      entry = util_find_env(argv[i]);
	      if (entry == NULL)
		return 1;
	      entry->set = 1;
	      if (entry->value)
		free (entry->value);
	      entry->value = NULL;
	    }
	}
      return 0;
    }
  return 1;
}
