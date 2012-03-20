/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2011-2012 Free Software Foundation, Inc.

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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mailutils/types.h>
#include <mailutils/error.h>
#include <mailutils/errno.h>
#include <mailutils/util.h>

#define SAFDEFAULT \
  (MU_FILE_SAFETY_GROUP_WRITABLE |		  \
   MU_FILE_SAFETY_WORLD_WRITABLE |		  \
   MU_FILE_SAFETY_GROUP_READABLE |		  \
   MU_FILE_SAFETY_WORLD_READABLE |		  \
   MU_FILE_SAFETY_LINKED_WRDIR   |		  \
   MU_FILE_SAFETY_DIR_IWGRP      |		  \
   MU_FILE_SAFETY_DIR_IWOTH      )
  
int
main (int argc, char **argv)
{
  int i;
  int input = 0;
  
  if (argc == 1)
    {
      mu_error ("usage: %s [+-]mode...", argv[0]);
      exit (1);
    }

  for (i = 1; i < argc; i++)
    {
      int rc = mu_file_safety_compose (&input, argv[i], SAFDEFAULT);
      if (rc)
	{
	  mu_error ("%s: %s", argv[i], mu_strerror (rc));
	  exit (1);
	}
    }
  printf ("%03o\n", mu_safety_criteria_to_file_mode (input));
  return 0;
}
