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
 * folders
 */

int
mail_folders (int argc, char **argv)
{
  char *path;
  struct mail_env_entry *env = util_find_env ("folder");

  (void)argc; (void)argv;

  if (!env->set)
    {
      util_error ("No value set for \"folder\"");
      return 1;
    }

  path = env->value;
  if (path[0] != '/' && path[0] != '~')
    {
      char *tmp = alloca (strlen (path) + 3);
      if (!tmp)
	{
	  util_error ("Not enough memory");
	  return 1;
	} 

      tmp[0] = '~';
      tmp[1] = '/';
      strcpy (tmp + 2, path);
      path = tmp;
    }
  
  path = util_fullpath (path);
  
  util_do_command("! %s %s", getenv ("LISTER"), path);
  free(path);

  return 0;
}
