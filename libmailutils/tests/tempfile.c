/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2005, 2007, 2009-2012 Free Software Foundation, Inc.

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
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <mailutils/util.h>
#include <mailutils/errno.h>
#include <mailutils/error.h>
#include <mailutils/stream.h>

char *progname;

void
usage (FILE *fp, int code)
{
  fprintf (fp,
	   "usage: %s [-tmpdir=DIR] [-suffix=SUF] [-dry-run | -unlink] { file | dir }\n",
	   progname);
  exit (code);
}

int
main (int argc, char **argv)
{
  struct mu_tempfile_hints hints, *phints;
  int flags = 0;
  int fd;
  int *pfd = &fd;
  char *filename;
  char **pname = &filename;
  char *infile = NULL;
  int verify = 0;
  int verbose = 0;
  int yes = 1;

  progname = argv[0];
  
  if (argc == 1)
    usage (stdout, 0);
  
  while (--argc)
    {
      char *arg = *++argv;

      if (strncmp (arg, "-tmpdir=", 8) == 0)
	{
	  hints.tmpdir = arg + 8;
	  flags |= MU_TEMPFILE_TMPDIR;
	}
      else if (strncmp (arg, "-suffix=", 8) == 0)
	{
	  hints.suffix = arg + 8;
	  flags |= MU_TEMPFILE_SUFFIX;
	}
      else if (strcmp (arg, "-dry-run") == 0)
	pfd = NULL;
      else if (strcmp (arg, "-unlink") == 0)
	pname = NULL;
      else if (strncmp (arg, "-infile=", 8) == 0)
	infile = arg + 8;
      else if (strncmp (arg, "-verify", 7) == 0)
	verify = 1;
      else if (strncmp (arg, "-verbose", 8) == 0)
	verbose = 1;
      else
	break;
    }

  if (argv[0] == NULL)
    usage (stderr, 1);
  if (strcmp (argv[0], "file") == 0)
    /* nothing */;
  else if (strcmp (argv[0], "dir") == 0)
    flags |= MU_TEMPFILE_MKDIR;
  else
    usage (stderr, 1);

  if (pname == NULL && pfd == NULL)
    {
      mu_error ("both -unlink and -dry-run given");
      exit (1);
    }

  if (infile)
    {
      if (flags & MU_TEMPFILE_MKDIR)
	{
	  mu_error ("-infile is useless with dir");
	  exit (1);
	}
      else if (pfd == NULL)
	{
	  mu_error ("-infile is useless with -dry-run");
	  exit (1);
	}
    }

  if (verify && pfd == NULL)
    {
      mu_error ("-verify is useless with -dry-run");
      exit (1);
    }
  
  phints = flags ? &hints : NULL;
  
  MU_ASSERT (mu_tempfile (phints, flags, pfd, pname));

  if (filename)
    printf ("created file name %s\n", filename);
    
  if (!pfd)
    return 0;
  
  if (infile)
    {
      mu_stream_t in, out;
      mu_off_t size;
      
      if (strcmp (infile, "-") == 0)
	MU_ASSERT (mu_stdio_stream_create (&in, MU_STDIN_FD, 0));
      else
	MU_ASSERT (mu_file_stream_create (&in, infile, MU_STREAM_READ));

      MU_ASSERT (mu_fd_stream_create (&out, filename, fd, MU_STREAM_WRITE));
      mu_stream_ioctl (out, MU_IOCTL_FD, MU_IOCTL_FD_SET_BORROW, &yes);
      MU_ASSERT (mu_stream_copy (out, in, 0, &size));
      if (verbose)
	printf ("copied %lu bytes to the temporary\n", (unsigned long) size);
      mu_stream_unref (out);
      mu_stream_unref (in);
    }

  if (verify)
    {
      mu_stream_t in, out;
      mu_off_t size;

      MU_ASSERT (mu_stdio_stream_create (&out, MU_STDOUT_FD, 0));
      MU_ASSERT (mu_fd_stream_create (&in, filename, fd,
				      MU_STREAM_READ|MU_STREAM_SEEK));
      mu_stream_ioctl (in, MU_IOCTL_FD, MU_IOCTL_FD_SET_BORROW, &yes);
      MU_ASSERT (mu_stream_copy (out, in, 0, &size));
      if (verbose)
	printf ("dumped %lu bytes\n", (unsigned long) size);
      mu_stream_unref (out);
      mu_stream_unref (in);
    }

  close (fd);
  
  return 0;
}

	  
