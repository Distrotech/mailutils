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

static list_t alternate_names = NULL;

/*
 * alt[ernates] name...
 */

int
mail_alt (int argc, char **argv)
{
  if (argc == 1)
    {
      if (alternate_names)
	{
	  util_slist_print (alternate_names, 0);
	  fprintf (ofile, "\n");
	}
    }
  else
    {
      util_slist_destroy (&alternate_names);
      while (--argc)
	util_slist_add (&alternate_names, *++argv);
    }
  return 0;
}

int
mail_is_alt_name (char *name)
{
  return util_slist_lookup (alternate_names, name);
}
