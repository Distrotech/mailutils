/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2003, 2005-2007, 2009-2012 Free Software
   Foundation, Inc.

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
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <mailutils/nls.h>
#include <mailutils/io.h>
#include <mailutils/error.h>
#include <mailutils/errno.h>
#include <mailutils/util.h>

#ifndef P_tmpdir
# define P_tmpdir "/tmp"
#endif

/* Lower level call.

   Create a temporary file with the name according to the pattern FILENAME.
   FILENAME must end with any amount of Xs, optionally followed by
   SUFLEN other characters.

   If ISDIR is not 0, create a directory (privileges 0700), otherwise
   create a file (privileges 0600).

   On success, return 0.  If pfd is not NULL (and ISDIR is 0), open the
   created file and store the descriptor in *PFD.  Return actual file
   name in FILENAME.

   On error, return error code.
*/   
int
mu_create_temp_file (char *filename, size_t suflen, int *pfd, int isdir)
{
  int rc;
  size_t len;
  char *carrybuf;
  char *p, *cp, *start, *end;
  struct stat st;
  static int first_call;
  static char randstate[256];
  static const unsigned char alphabet[] =
    "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

  if (!first_call)
    {
      /* Initialize random number generator */
      struct timeval tv;
      gettimeofday (&tv, NULL);
      initstate (((unsigned long) tv.tv_usec << 16) ^ tv.tv_sec,
		 randstate, sizeof (randstate));
      first_call = 1;
    }
  setstate (randstate);
  
  /* Start with the last filename character before suffix */
  end = filename + strlen (filename) - suflen - 1;
  /* Fill X's with random characters */
  for (p = end; p >= filename && *p == 'X'; p--)
    *p = alphabet[random () % (sizeof (alphabet) - 1)];
  len = end - p;
  if (len == 0)
    return EINVAL;
  start = p + 1;

  carrybuf = malloc (len);
  if (!carrybuf)
    return ENOMEM;

  /* Fill in the carry buffer */
  memcpy (carrybuf, start, len);

  for (;;)
    {
      if (isdir)
	{
	  if (mkdir (filename, 0700) == 0)
	    {
	      rc = 0;
	      break;
	    }
	}
      else if (pfd)
	{
	  if ((*pfd = open (filename, O_CREAT|O_EXCL|O_RDWR, 0600)) >= 0)
	    {
	      rc = 0;
	      break;
	    }
	}
      else if (lstat (filename, &st) && errno == ENOENT)
	{
	  rc = 0;
	  break;
	}
      
      if (errno != EEXIST)
	{
	  rc = errno;
	  break;
	}

      for (p = start, cp = carrybuf;; p++, cp++)
	{
	  char *q;
	  
	  if (p == end)
	    /* All permutation exhausted */
	    return EEXIST;
	  q = strchr ((char*)alphabet, *p);
	  if (!q)
	    abort (); /* should not happen */
	  *p = (q[1] == 0) ? alphabet[0] : q[1];
	  if (*p != *cp)
	    break;
	}
    }
  free (carrybuf);
  return rc;
}

/* Create a temporary file according to HINTS.

   If MU_TEMPFILE_MKDIR flag is set, create a directory instead of file.

   On success, return 0.  If PFD is not NULL (and MU_TEMPFILE_MKDIR flag
   is not set), open the file and store its descriptor in *PFD.  If
   NAMEP is not NULL, store there a pointer to the allocated file name.
   At least one of PFD and NAMEP must be set, otherwise EINVAL is returned.

   On error, returns error code describing the problem.

   If hints is NULL, any flags except MU_TEMPFILE_MKDIR are ignored.
   Otherwise:
   
   * If MU_TEMPFILE_TMPDIR is set, hints->tmpdir points to the name of
     a directory where to create the temporary.
   * If MU_TEMPFILE_SUFFIX is set, hints->suffix defines a suffix to
     append to the created file name.
 */
int
mu_tempfile (struct mu_tempfile_hints *hints, int flags,
	     int *pfd, char **namep)
{
  char *filename;
  const char *tmpdir = getenv ("TMPDIR");
  const char *suf = NULL;
  int create_dir = 0;
  int rc;
  struct stat st;

  if (pfd == NULL && namep == NULL)
    return EINVAL;
  if (hints)
    {
      if (flags & MU_TEMPFILE_TMPDIR)
	tmpdir = hints->tmpdir;
      if (flags & MU_TEMPFILE_SUFFIX)
	suf = hints->suffix;
      create_dir = flags & MU_TEMPFILE_MKDIR;
    }
      
  if (!tmpdir)
    tmpdir = P_tmpdir;

  /* First, see if tmpdir exists */
  if (stat (tmpdir, &st))
    return errno;

  /* Create a name template */
  filename = mu_make_file_name_suf (tmpdir, "muXXXXXX", suf);
  rc = mu_create_temp_file (filename, suf ? strlen (suf) : 0, pfd, create_dir);
  if (rc == 0)
    {
      if (namep)
	*namep = filename;
      else
	{
	  unlink (filename);
	  free (filename);
	}
    }
  return rc;
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
  struct mu_tempfile_hints hints;
  char *filename = NULL;
  int fd;
  hints.tmpdir = (char*)tmpdir;
  if (mu_tempfile (&hints, MU_TEMPFILE_TMPDIR, &fd, &filename))
    return NULL;
  close (fd);
  return filename;
}

