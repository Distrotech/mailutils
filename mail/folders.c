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
#include <dirent.h>

/*
 * folders
 */

int
mail_folders (int argc, char **argv)
{
  DIR *dir;
  struct dirent *dirent;
  char *path;
  struct mail_env_entry *env = util_find_env ("folder");

  if (!env->set)
    {
      fprintf(ofile, "No value set for \"folder\"\n");
      return 1;
    }

  path = util_fullpath(env->value);
  dir = opendir(path);
  if (!dir)
    {
      fprintf(ofile, "can't open directory `%s'\n", path);
      free(path);
      return 1;
    }
  

  while (dirent = readdir(dir))
    {
      if (dirent->d_name[0] == '.')
	continue;
      fprintf(ofile, "%s\n", dirent->d_name);
    }
  
  closedir(dir);

  free(path);
  
  return 0;
}
