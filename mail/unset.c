/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001, 2007, 2010-2012 Free Software Foundation,
   Inc.

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

/*
 * uns[et] [name...] -- GNU extension
 */

int
mail_unset (int argc, char **argv)
{
  if (argc < 2)
    {
      mailvar_print (0);
      return 0;
    }
  else
    {
      int status = 0, i = 1;
      for (i = 1; i < argc; i++)
	{
	  char *buf = mu_alloc ((7+strlen (argv[i])) * sizeof (char));
	  strcpy (buf, "set no");
	  strcat (buf, argv[i]);
	  if (!util_do_command ("%s", buf))
	    status = 1;
	  free (buf);
	}
      return status;
    }
  return 1;
}
