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

#define SAFMASK (				\
  MU_FILE_SAFETY_GROUP_WRITABLE |		\
  MU_FILE_SAFETY_WORLD_WRITABLE |		\
  MU_FILE_SAFETY_GROUP_READABLE |		\
  MU_FILE_SAFETY_WORLD_READABLE )

int
main (int argc, char **argv)
{
  int i, crit;
  
  if (argc != 2)
    {
      mu_error ("usage: %s mode", argv[0]);
      exit (1);
    }

  crit = mu_file_mode_to_safety_criteria (strtoul (argv[1], NULL, 8)) &
         SAFMASK;
  for (i = 1; crit && i != 0; i <<= 1)
    {
      if (crit & i)
	{
	  const char *s = mu_file_safety_code_to_name (i);
	  printf ("%s\n", s ? s : "UNKNOWN");
	}
      crit &= ~i;
    }
  return 0;
}
