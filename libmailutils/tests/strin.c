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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <mailutils/stream.h>
#include <mailutils/stdstream.h>
#include <mailutils/diag.h>
#include <mailutils/errno.h>

int
main (int argc, char **argv)
{
  int i, rc;
  int echo_state = 0;
  size_t n;
  char buf[80];

  for (i = 1; i < argc; i++)
    {
      char *arg = argv[i];
      if (arg[0] == '-')
	{
	  if (strcmp (arg, "-noecho") == 0)
	    {
	      MU_ASSERT (mu_stream_ioctl (mu_strin, MU_IOCTL_ECHO,
					  MU_IOCTL_OP_SET,
					  &echo_state));
	      echo_state = 1;
	    }
	  else
	    {
	      fprintf (stderr, "usage: %s [-noecho]\n", argv[0]);
	      return 1;
	    }
	}
    }
	
  while ((rc = mu_stream_read (mu_strin, buf, sizeof (buf), &n) == 0) &&
	 n > 0)
    fwrite (buf, 1, n, stdout);

  if (echo_state)
    MU_ASSERT (mu_stream_ioctl (mu_strin, MU_IOCTL_ECHO, MU_IOCTL_OP_SET,
				&echo_state));
  return 0;
}
