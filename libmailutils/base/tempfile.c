/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002, 2003, 2005, 2006, 2007, 2009,
   2010 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library.  If not, see 
   <http://www.gnu.org/licenses/>. */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <mailutils/nls.h>
#include <mailutils/io.h>
#include <mailutils/error.h>
#include <mailutils/errno.h>

/* Create and open a temporary file. Be very careful about it, since we
   may be running with extra privilege i.e setgid().
   Returns file descriptor of the open file.
   If namep is not NULL, the pointer to the malloced file name will
   be stored there. Otherwise, the file is unlinked right after open,
   i.e. it will disappear after close(fd). */

#ifndef P_tmpdir
# define P_tmpdir "/tmp"
#endif

int
mu_tempfile (const char *tmpdir, char **namep)
{
  char *filename;
  int fd;
  
  if (!tmpdir)
    tmpdir = (getenv ("TMPDIR")) ? getenv ("TMPDIR") : P_tmpdir;

  if (mu_asprintf (&filename, "%s/muXXXXXX", tmpdir))
    return -1;

#ifdef HAVE_MKSTEMP
  {
    int save_mask = umask (077);
    fd = mkstemp (filename);
    umask (save_mask);
  }
#else
  if (mktemp (filename))
    fd = open (filename, O_CREAT|O_EXCL|O_RDWR, 0600);
  else
    fd = -1;
#endif

  if (fd == -1)
    {
      mu_error (_("cannot open temporary file: %s"), mu_strerror (errno));
      free (filename);
      return -1;
    }

  if (namep)
    *namep = filename;
  else
    {
      unlink (filename);
      free (filename);
    }

  return fd;
}

/* Create a unique temporary file name in tmpdir. The function
   creates an empty file with this name to avoid possible race
   conditions. Returns a pointer to the malloc'ed file name.
   If tmpdir is NULL, the value of the environment variable
   TMPDIR or the hardcoded P_tmpdir is used, whichever is defined. */
/* FIXME: Remove it. Use mu_tempfile instead. */
char *
mu_tempname (const char *tmpdir)
{
  char *filename = NULL;
  int fd = mu_tempfile (tmpdir, &filename);
  close (fd);
  return filename;
}


