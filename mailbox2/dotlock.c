/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <errno.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <utime.h>
#include <signal.h>

#include <mailutils/error.h>
#include <mailutils/sys/dotlock.h>
#include <mailutils/refcount.h>

/* First draft by Brian Edmond. */

int
_lockfile_dotlock_ref (lockfile_t lockfile)
{
  struct _lockfile_dotlock *dotlock = (struct _lockfile_dotlock *)lockfile;
  return mu_refcount_inc (dotlock->refcount);
}

void
_lockfile_dotlock_destroy (lockfile_t *plockfile)
{
  struct _lockfile_dotlock *dotlock = (struct _lockfile_dotlock *)*plockfile;
  if (mu_refcount_dec (dotlock->refcount) == 0)
    {
      mu_refcount_destroy (&dotlock->refcount);
      if (dotlock->fname)
	free (dotlock->fname);
      free (dotlock);
    }
}

int
_lockfile_dotlock_lock (lockfile_t lockfile)
{
  struct _lockfile_dotlock *dotlock = (struct _lockfile_dotlock *)lockfile;
  int   fd = -1;
  char  buf[16];
  pid_t pid;
  int   removed = 0;

  if (dotlock == NULL)
    return MU_ERROR_INVALID_PARAMETER;

  /* Is the lock already applied?
     FIXME: should we check flags != lock->flags ?? */
  if (dotlock->fd != -1)
    {
      dotlock->refcnt++;
      return 0;
    }

  /*
    Check for lock existance:
    if it exists but the process is gone the lock can be removed,
    if the lock is expired, remove it.  */
  fd = open (dotlock->fname, O_RDONLY);
  if (fd != -1)
    {
      /* Check to see if this process is still running.  */
      if (dotlock->flags & MU_LOCKFILE_DOTLOCK_PID)
        {
	  int nread = read (fd, buf, sizeof (buf) - 1);
          if (nread > 0)
            {
	      buf[nread] = '\0';
              switch (pid = strtol (buf, NULL, 10))
		{
		case LONG_MIN:
		case LONG_MAX:
		  if (errno == ERANGE)
		    removed = 1;
		  break;
		default:
		  /* Process is gone so we try to remove the lock.  */
		  if (kill (pid, 0) == -1)
		    removed = 1;
		}
	    }
        }
      /* Check to see if the lock expired.  */
      if (dotlock->flags & MU_LOCKFILE_DOTLOCK_TIME)
        {
          struct stat stbuf;

          fstat (fd, &stbuf);
          /* The lock has expired.  */
          if ((time (NULL) - stbuf.st_mtime) > MU_LOCKFILE_DOTLOCK_EXPIRE_TIME)
            removed = 1;
        }

      close (fd);
      if (removed)
        unlink (dotlock->fname);
    }

  /* Try to create the lockfile.  */
  fd = open (dotlock->fname, O_WRONLY | O_CREAT | O_EXCL, MU_LOCKFILE_DOTLOCK_ATTR);
  if (fd == -1)
    return errno;
  else
    {
      struct stat fn_stat;
      struct stat fd_stat;

      if (lstat (dotlock->fname, &fn_stat)
	  || fstat(fd, &fd_stat)
	  || fn_stat.st_nlink != 1
	  || fn_stat.st_dev != fd_stat.st_dev
	  || fn_stat.st_ino != fd_stat.st_ino
	  || fn_stat.st_uid != fd_stat.st_uid
	  || fn_stat.st_gid != fd_stat.st_gid)
	{
	  close (fd);
	  unlink (dotlock->fname);
	  return EPERM;
	}
    }

  /* Success.  */
  sprintf (buf, "%ld", (long)getpid ());
  write (fd, buf, strlen (buf));

  /* Try to get a file lock.  */
  if (dotlock->flags & MU_LOCKFILE_DOTLOCK_FCNTL)
    {
      struct flock fl;

      memset (&fl, 0, sizeof (struct flock));
      fl.l_type = F_WRLCK;
      if (fcntl (fd, F_SETLK, &fl) == -1)
        {
	  int err = errno;
          /* Could not get the file lock.  */
          close (fd);
          unlink (dotlock->fname); /* Remove the file I created.  */
          return err;
        }
    }

  dotlock->fd = fd;
  dotlock->refcnt++;
  return 0;
}

int
_lockfile_dotlock_touchlock (lockfile_t lockfile)
{
  struct _lockfile_dotlock *dotlock = (struct _lockfile_dotlock *)lockfile;
  if (!dotlock || !dotlock->fname || dotlock->fd == -1)
    return MU_ERROR_INVALID_PARAMETER;
  return utime (dotlock->fname, NULL);
}

int
_lockfile_dotlock_unlock (lockfile_t lockfile)
{
  struct _lockfile_dotlock *dotlock = (struct _lockfile_dotlock *)lockfile;
  if (!dotlock || !dotlock->fname || dotlock->fd == -1 || dotlock->refcnt <= 0)
    return EINVAL;

  if (--dotlock->refcnt > 0)
    return 0;

  if (dotlock->flags & MU_LOCKFILE_DOTLOCK_FCNTL)
    {
      struct flock fl;

      memset (&fl, 0, sizeof (struct flock));
      fl.l_type = F_UNLCK;
      /* Unlock failed?  */
      if (fcntl (dotlock->fd, F_SETLK, &fl) == -1)
          return errno;
    }
  close (dotlock->fd);
  dotlock->fd = -1;
  unlink (dotlock->fname);
  return 0;
}

static struct _lockfile_vtable _lockfile_dotlock_vtable =
{
  _lockfile_dotlock_ref,
  _lockfile_dotlock_destroy,

  _lockfile_dotlock_lock,
  _lockfile_dotlock_touchlock,
  _lockfile_dotlock_unlock,
};

int
_lockfile_dotlock_ctor (struct _lockfile_dotlock *dotlock,
			const char *filename)
{
  mu_refcount_create (&dotlock->refcount);
  if (dotlock->refcount == NULL)
    return MU_ERROR_NO_MEMORY;

  dotlock->fname = calloc (strlen (filename) + 5 /*strlen(".lock")*/ + 1, 1);
  if (dotlock->fname == NULL)
    {
      mu_refcount_destroy (&dotlock->refcount);
      return MU_ERROR_NO_MEMORY;
    }
  strcpy (dotlock->fname, filename);
  strcat (dotlock->fname, ".lock");

  dotlock->flags = MU_LOCKFILE_DOTLOCK_PID | MU_LOCKFILE_DOTLOCK_TIME
    | MU_LOCKFILE_DOTLOCK_FCNTL;
  dotlock->fd = -1;
  dotlock->refcnt = 0;
  dotlock->base.vtable = &_lockfile_dotlock_vtable;
  return 0;
}

void
_lockfile_dotlock_dtor (lockfile_t lockfile)
{
  struct _lockfile_dotlock *dotlock = (struct _lockfile_dotlock *)lockfile;
  if (dotlock)
    {
      mu_refcount_destroy (&dotlock->refcount);
      if (dotlock->fname)
	free (dotlock->fname);
    }
}

int
lockfile_dotlock_create (lockfile_t *plockfile, const char *filename)
{
  struct _lockfile_dotlock *dotlock;
  int status;

  if (plockfile == NULL || filename == NULL)
    return MU_ERROR_INVALID_PARAMETER;

  dotlock = calloc (1, sizeof *dotlock);
  if (dotlock == NULL)
    return MU_ERROR_NO_MEMORY;

  status = _lockfile_dotlock_ctor (dotlock, filename);
  if (status != 0)
    {
      free (dotlock);
      return status;
    }
  *plockfile = &dotlock->base;
  return 0;
}
