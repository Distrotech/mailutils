/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001-2002, 2005, 2007, 2010-2012 Free Software
   Foundation, Inc.

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
 * folders
 */

int
mail_folders (int argc MU_ARG_UNUSED, char **argv MU_ARG_UNUSED)
{
  char *path;

  if (mailvar_get (&path, "folder", mailvar_type_string, 1))
    return 1;

  if (path[0] != '/' && path[0] != '~')
    {
      char *tmp = mu_alloc (strlen (path) + 3);
      tmp[0] = '~';
      tmp[1] = '/';
      strcpy (tmp + 2, path);
      path = util_fullpath (tmp);
      free (tmp);
    }
  else
    path = util_fullpath (path);
  
  util_do_command("! %s '%s'", getenv ("LISTER"), path);
  free (path);

  return 0;
}
