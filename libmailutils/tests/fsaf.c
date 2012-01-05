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
#include <unistd.h>
#include <mailutils/types.h>
#include <mailutils/error.h>
#include <mailutils/errno.h>
#include <mailutils/util.h>

int
main (int argc, char **argv)
{
  int i;
  int mode = MU_FILE_SAFETY_OWNER_MISMATCH, rc, m;
  mu_list_t idlist;
  uid_t uid;

  if (argc == 1)
    {
      mu_error ("usage: %s [+-]mode... file...", argv[0]);
      exit (1);
    }
  
  rc = mu_list_create (&idlist);
  if (rc)
    {
      mu_error ("mu_list_create: %s", mu_strerror (rc));
      exit (1);
    }
  uid = getuid ();
  
  for (i = 1; i < argc; i++)
    {
      if (argv[i][0] == '-' || argv[i][0] == '+')
	{
	  rc = mu_file_safety_name_to_code (argv[i] + 1, &m);
	  if (rc)
	    {
	      mu_error ("%s: %s", argv[i], mu_strerror (rc));
	      exit (1);
	    }
	  if (argv[i][0] == '-')
	    mode &= ~m;
	  else
	    mode |= m;
	}
      else
	{
	  rc = mu_file_safety_check (argv[i], mode, uid, idlist);
	  printf ("%s: %s\n", argv[i], mu_strerror (rc));
	}
    }
  exit (0);
}
