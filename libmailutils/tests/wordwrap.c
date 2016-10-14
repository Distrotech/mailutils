/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2011-2012, 2014-2016 Free Software Foundation, Inc.

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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mailutils/stream.h>
#include <mailutils/stdstream.h>
#include <mailutils/diag.h>
#include <mailutils/debug.h>
#include <mailutils/errno.h>

static void
usage (FILE *fp)
{
  fprintf (fp,
	   "usage: %s [-r N] [-l [+-]N] FILE [[-l [+-]N] FILE...]\n",
	   mu_program_name);
  fprintf (fp, "\n");
  fprintf (fp, "  -l N       set left margin\n");
  fprintf (fp, "  -l +N      move left margin N chars to the right from the current position\n");
  fprintf (fp, "  -l -N      move left margin N chars to the left from the current position\n");
  fprintf (fp, "  -r N       set right margin\n");
  fprintf (fp, "  -h, -?     display this help\n");
}

int
main (int argc, char **argv)
{
  unsigned left = 0, right = 80;
  mu_stream_t str;
  int i;
  
  mu_set_program_name (argv[0]);
 
  for (i = 1; i < argc; i++)
    {
      char *arg = argv[i];
      if (strncmp (arg, "-l", 2) == 0)
	{
	  if (arg[2] == 0)
	    {
	      ++i;
	      left = strtoul (argv[i], NULL, 10);
	    }
	  else
	    {
	      left = strtoul (argv[i] + 2, NULL, 10);
	    }
	}
      else if (strncmp (arg, "-r", 2) == 0)
	{
	  if (arg[2] == 0)
	    {
	      ++i;
	      right = strtoul (argv[i], NULL, 10);
	    }
	  else
	    {
	      right = strtoul (argv[i] + 2, NULL, 10);
	    }
	}
      else if (strcmp (arg, "--") == 0)
	{
	  i++;
	  break;
	}
      else if (arg[0] == '-')
	{
	  if (arg[1] == 0)
	    break;
	  else if ((arg[1] == 'h' || arg[1] == '?') && arg[2] == 0)
	    {
	      usage (stdout);
	      return 0;
	    }
	  else
	    {
	      fprintf (stderr, "%s: unrecognized argument %s\n",
		       mu_program_name, arg);
	      usage (stderr);
	      return 1;
	    }
	}
      else
	break;
    }

  if (i == argc)
    {
      fprintf (stderr, "%s: no files\n", mu_program_name);
      usage (stderr);
      return 1;
    }
  
  mu_stdstream_setup (MU_STDSTREAM_RESET_NONE);
  MU_ASSERT (mu_wordwrap_stream_create (&str, mu_strout, left, right));

  for (; i < argc; i++)
    {
      char *arg = argv[i];
      if (strncmp (arg, "-l", 2) == 0)
	{
	  if (arg[2] == 0)
	    {
	      ++i;
	      arg = argv[i];
	    }
	  else
	    arg += 2;

	  if (arg[0] == '+' || arg[0] == '-')
	    {
	      int off = strtol (arg, NULL, 10);
	      MU_ASSERT (mu_stream_ioctl (str, MU_IOCTL_WORDWRAPSTREAM,
					  MU_IOCTL_WORDWRAP_MOVE_MARGIN,
					  &off));
	    }
	  else
	    {
	      left = strtoul (arg, NULL, 10);
	      MU_ASSERT (mu_stream_ioctl (str, MU_IOCTL_WORDWRAPSTREAM,
					  MU_IOCTL_WORDWRAP_SET_MARGIN,
					  &left));
	    }
	}
      else if (arg[0] == '-' && arg[1] == 0)
	{
	  MU_ASSERT (mu_stream_copy (str, mu_strin, 0, NULL));
	  mu_stream_close (mu_strin);
	}
      else
	{
	  mu_stream_t in;
	  MU_ASSERT (mu_file_stream_create (&in, arg, MU_STREAM_READ));
	  MU_ASSERT (mu_stream_copy (str, in, 0, NULL));
	  mu_stream_destroy (&in);
	}
    }
  mu_stream_destroy (&str);
  return 0;
}
    

  
	    
