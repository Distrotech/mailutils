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
 * l[ist]
 * *
 */

int
mail_list (int argc, char **argv)
{
  char *cmd = NULL;
  int i = 0, pos = 0, len = 0;
  int cols = util_getcols ();

  for (i=0; mail_command_table[i].shortname != 0; i++)
    {
      len = strlen (mail_command_table[i].longname);
      if (len < 1)
	{
	  cmd = mail_command_table[i].shortname;
	  len = strlen (cmd);
	}
      else
	cmd = mail_command_table[i].longname;

      pos += len + 1;

      if (pos >= cols)
	{
	  pos = len + 1;
	  printf ("\n%s ", cmd);
	}
      else
        printf ("%s ", cmd);
    }
  printf ("\n");
  return 0;
}
