/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003, 2005, 2007, 2010-2012 Free Software Foundation,
   Inc.

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
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <mailutils/mailutils.h>
#include <mailutils/sys/prog_stream.h>

int
main (int argc, char *argv[])
{
  int rc;
  mu_stream_t stream, out;
  int read_stdin = 0;
  int i;
  int flags = MU_STREAM_READ;
  struct mu_prog_hints hints;
  int hint_flags = 0;
  char *progname = NULL;
  gid_t gid[20];
  size_t gn = 0;
  
  for (i = 1; i < argc; i++)
    {
      if (strcmp (argv[i], "--stdin") == 0)
	{
	  read_stdin = 1;
	  flags |= MU_STREAM_WRITE;
	}
      else if (strcmp (argv[i], "--chdir") == 0)
	{
	  hints.mu_prog_workdir = argv[i+1];
	  hint_flags |= MU_PROG_HINT_WORKDIR;
	  i++;
	}
      else if (strncmp (argv[i], "--limit", 7) == 0
	       && mu_isdigit (argv[i][7]))
	{
	  int n;

	  if (i + 1 == argc)
	    {
	      fprintf (stderr, "%s requires argument\n", argv[i]);
	      exit (1);
	    }
	  n = argv[i][7] - '0';
	  if (!(_mu_prog_limit_flags & MU_PROG_HINT_LIMIT(n)))
	    {
	      fprintf (stderr, "%s is not supported\n", argv[i]+2);
	      continue;
	    }
	  hint_flags |= MU_PROG_HINT_LIMIT(n);
	  hints.mu_prog_limit[n] = strtoul (argv[i+1], NULL, 10);
	  i++;
	}
      else if (strcmp (argv[i], "--prio") == 0)
	{
	  if (i + 1 == argc)
	    {
	      fprintf (stderr, "%s requires argument\n", argv[i]);
	      exit (1);
	    }
	  hint_flags |= MU_PROG_HINT_PRIO;
	  hints.mu_prog_prio = strtoul (argv[i+1], NULL, 10);
	  i++;
	}
      else if (strcmp (argv[i], "--exec") == 0)
	{
	  if (i + 1 == argc)
	    {
	      fprintf (stderr, "%s requires argument\n", argv[i]);
	      exit (1);
	    }
	  progname = argv[++i];
	}
      else if (strcmp (argv[i], "--errignore") == 0)
	hint_flags |= MU_PROG_HINT_IGNOREFAIL;
      else if (strcmp (argv[i], "--uid") == 0)
	{
	  if (i + 1 == argc)
	    {
	      fprintf (stderr, "%s requires argument\n", argv[i]);
	      exit (1);
	    }
	  hint_flags |= MU_PROG_HINT_UID;
	  hints.mu_prog_uid = strtoul (argv[i+1], NULL, 10);
	  i++;
	}
      else if (strcmp (argv[i], "--gid") == 0)
	{
	  mu_list_t list;
	  mu_iterator_t itr;
	  
	  if (i + 1 == argc)
	    {
	      fprintf (stderr, "%s requires argument\n", argv[i]);
	      exit (1);
	    }
	  mu_list_create (&list);
	  mu_list_set_destroy_item (list, mu_list_free_item);
	  rc = mu_string_split (argv[++i], ",", list);
	  if (mu_list_get_iterator (list, &itr) == 0)
	    {
	      for (mu_iterator_first (itr);
		   !mu_iterator_is_done (itr); mu_iterator_next (itr))
		{
	          char *p;

		  mu_iterator_current (itr, (void**)&p);
		  if (gn >= MU_ARRAY_SIZE (gid))
		    {
		      fprintf (stderr, "too many gids\n");
		      exit (1);
		    }
		  gid[gn++] = strtoul (p, NULL, 10);
		}
	      mu_iterator_destroy (&itr);
	    }
	  else
	    {
	      mu_diag_funcall (MU_DIAG_ERROR, "mu_list_get_iterator", NULL,
			       rc);
	      exit (1);
	    }
	  mu_list_destroy (&list);
	  hint_flags |= MU_PROG_HINT_GID;
	}
      else if (strcmp (argv[i], "--") == 0)
	{
	  i++;
	  break;
	}
      else
	break;
    }
  
  if (i == argc)
    {
      fprintf (stderr,
	       "Usage: %s [--stdin] [--chdir dir] [--limit{0-9} lim] [--prio N]\n"
	       "          [--exec progname] progname [args]\n", argv[0]);
      exit (1);
    }

  argc -= i;
  argv += i;

  if (!progname)
    progname = argv[0];
  
  if (read_stdin)
    {
      MU_ASSERT (mu_stdio_stream_create (&hints.mu_prog_input,
					 MU_STDIN_FD, 0));
      hint_flags |= MU_PROG_HINT_INPUT;
    }

  rc = mu_prog_stream_create (&stream, progname, argc, argv,
			      hint_flags, &hints, flags);
  if (hint_flags & MU_PROG_HINT_INPUT)
    /* Make sure closing/destroying stream will close/destroy input */
    mu_stream_unref (hints.mu_prog_input);

  if (rc)
    {
      fprintf (stderr, "%s: cannot create program filter stream: %s\n",
	       argv[0], mu_strerror (rc));
      exit (1);
    }
  
  MU_ASSERT (mu_stdio_stream_create (&out, MU_STDOUT_FD, 0));

  mu_stream_copy (out, stream, 0, NULL);
  mu_stream_close (stream);
  mu_stream_destroy (&stream);
  mu_stream_close (out);
  mu_stream_destroy (&out);
  return 0;
}
