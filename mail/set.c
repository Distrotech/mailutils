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
      char *value = NULL;
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
	      entry->value = NULL;
	    }
	  else if (i+1 < argc && argv[i+1][0] == '=')
	    {
	      entry = util_find_env (argv[i]);
	      if (entry == NULL)
		return 1;
	      i += 2;
	      if (i >= argc)
		break;
	      value = strdup (argv[i]);
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
