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
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#include "mail.h"

/*
 * l[ist]
 * *
 */

int
mail_list (int argc, char **argv)
{
  const char *cmd = NULL;
  int i = 0, pos = 0, len = 0;
  int cols = util_getcols ();

  (void)argc; (void)argv;

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
	  fprintf (ofile, "\n%s ", cmd);
	}
      else
        fprintf (ofile, "%s ", cmd);
    }
  fprintf (ofile, "\n");
  return 0;
}
