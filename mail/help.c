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
#include "table.h"

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
      while (mail_command_table[i].synopsis != 0)
	printf ("%s\n", mail_command_table[i++].synopsis);
      return 0;
    }
  else
    {
      int status = 0;
      int command = 0;
      while (++command < argc)
	{
	  char *cmd = argv[command];
	  int i = 0, printed = 0, sl = 0, ll = 0, len = strlen (cmd);
	  while (mail_command_table[i].shortname != 0 && printed == 0)
	    {
	      sl = strlen (mail_command_table[i].shortname);
	      ll = strlen (mail_command_table[i].longname);
	      if (sl == len && !strcmp (mail_command_table[i].shortname, cmd))
		{
		  printed = 1;
		  printf ("%s\n", mail_command_table[i].synopsis);
		}
	      else if (sl < len && !strncmp (mail_command_table[i].longname,
					     cmd, len))
		{
		  printed = 1;
		  printf ("%s\n", mail_command_table[i].synopsis);
		}
	      i++;
	    }
	  if (printed == 0)
	    {
	      printf ("Unknown command: %s\n", cmd);
	      status = 1;
	    }
	}
    }
  return 1;
}
