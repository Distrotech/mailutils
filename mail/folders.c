/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001, 2002 Free Software Foundation, Inc.

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
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA  */

#include "mail.h"

/*
 * folders
 */

int
mail_folders (int argc ARG_UNUSED, char **argv ARG_UNUSED)
{
  char *path;

  if (util_getenv (&path, "folder", Mail_env_string, 1))
    return 1;

  if (path[0] != '/' && path[0] != '~')
    {
      char *tmp = alloca (strlen (path) + 3);
      if (!tmp)
	{
	  util_error (_("Not enough memory"));
	  return 1;
	} 

      tmp[0] = '~';
      tmp[1] = '/';
      strcpy (tmp + 2, path);
      path = tmp;
    }
  
  path = util_fullpath (path);
  
  util_do_command("! %s '%s'", getenv ("LISTER"), path);
  free(path);

  return 0;
}
