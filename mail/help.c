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
 * hel[p] [command]
 * ? [command]
 */

int
mail_help (int argc, char **argv)
{
  if (argc < 2)
    {
      int i = 0;
      FILE *out = ofile;

      if ((util_find_env("crt"))->set)
	out = popen (getenv("PAGER"), "w");

      while (mail_command_table[i].synopsis != 0)
	fprintf (out, "%s\n", mail_command_table[i++].synopsis);

      if (out != ofile)
	pclose (out);

      return 0;
    }
  else
    {
      int status = 0, cmd = 0;

      while (++cmd < argc)
	{
	  struct mail_command_entry entry = util_find_entry (argv[cmd]);
	  if (entry.synopsis != NULL)
	    fprintf (ofile, "%s\n", entry.synopsis);
	  else
	    {
	      status = 1;
	      fprintf (ofile, "Unknown command: %s\n", argv[cmd]);
	    }
	}
      return status;
    }
  return 1;
}
