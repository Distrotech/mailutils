/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Library Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <utime.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <mailutils/locker.h>
#include <mailutils/errno.h>

#define LOCKFILE_ATTR           0444

/* First draft by Brian Edmond. */
/* For subsequent modifications, see the GNU mailutils ChangeLog. */

struct _locker
{
  int fd;
  int refcnt;

  char *file;

  char *dotlock;

  int flags;

  int expire_time;
  int retries;
  int retry_sleep;
};

/* Assert that we're managing the refcnt and fd correctly, either
 * we have a lock, and the fd is valid, or refcnt is 0 and fd is -1.
 * And refcnt can never be less than 0.
 */
#define INVARIANT(l) \
  assert((l)->refcnt >= 0); \
  assert( \
      (((l)->refcnt >  0) && ((l)->fd != -1)) || \
      (((l)->refcnt == 0) && ((l)->fd == -1)) \
      );

int
locker_create (locker_t *plocker, const char *filename, int flags)
{
  locker_t l;

  if (plocker == NULL)
    return MU_ERR_OUT_PTR_NULL;

  if(filename == NULL)
    return EINVAL;

  l = calloc (1, sizeof (*l));
  if (l == NULL)
    return ENOMEM;

  /* Should make l->file be the resulting of following the symlinks. */
  l->file = strdup(filename);
  if (l->file == NULL)
    {
      free (l);
      return ENOMEM;
    }

  l->dotlock = malloc(strlen(l->file) + 5 /*strlen(".lock")*/ + 1);

  if(!l->dotlock)
  {
    free(l->file);
    free(l);
    return ENOMEM;
  }

  sprintf(l->dotlock, "%s.lock", l->file);

  if (flags)
    l->flags = flags;
  else
    l->flags = MU_LOCKER_DEFAULT;

  l->fd = -1;
  l->expire_time = MU_LOCKER_EXPIRE_TIME;
  l->retries = MU_LOCKER_RETRIES;
  l->retry_sleep = MU_LOCKER_RETRY_SLEEP;

  INVARIANT(l)

  *plocker = l;

  return 0;
}

void
locker_destroy (locker_t *plocker)
{
  if (plocker && *plocker)
    {
      close((*plocker)->fd);
      free ((*plocker)->file);
      free ((*plocker)->dotlock);
      free (*plocker);
      *plocker = NULL;
    }
}

int locker_set_flags (locker_t locker, int flags)
{
  if (!locker)
    return MU_ERR_LOCKER_NULL;

  locker->flags = flags;

  return 0;
}

int
locker_set_expire_time (locker_t locker, int etime)
{
  if (!locker)
    return MU_ERR_LOCKER_NULL;

  if(etime <= 0)
    return EINVAL;

  locker->expire_time = etime;

  return 0;
}

int
locker_set_retries (locker_t locker, int retries)
{
  if (!locker)
    return MU_ERR_LOCKER_NULL;

  if(retries <= 0)
    return EINVAL;

  locker->retries = retries;

  return 0;
}

int
locker_set_retry_sleep (locker_t locker, int retry_sleep)
{
  if (!locker)
    return MU_ERR_LOCKER_NULL;

  if(retry_sleep <= 0)
    return EINVAL;

  locker->retry_sleep = retry_sleep;

  return 0;
}

int locker_get_flags (locker_t locker, int *flags)
{
  if (!locker)
    return MU_ERR_LOCKER_NULL;

  if(!flags)
    return EINVAL;

  *flags = locker->flags;

  return 0;
}

int
locker_get_expire_time (locker_t locker, int *ptime)
{
  if (!locker)
    return MU_ERR_LOCKER_NULL;

  if(!ptime)
    return EINVAL;

  *ptime = locker->expire_time;

  return 0;
}

int
locker_get_retries (locker_t locker, int *retries)
{
  if (!locker)
    return MU_ERR_LOCKER_NULL;

  if(!retries)
    return EINVAL;

  *retries = locker->retries;

  return 0;
}

int
locker_get_retry_sleep (locker_t locker, int *retry_sleep)
{
  if (!locker)
    return MU_ERR_LOCKER_NULL;

  if(!retry_sleep)
    return EINVAL;

  *retry_sleep = locker->retry_sleep;

  return 0;
}

/* expire a stale lock (if MU_LOCKER_PID or MU_LOCKER_TIME) */
static void
expire_stale_lock (locker_t lock)
{
  int stale = 0;
  int fd = open (lock->dotlock, O_RDONLY);
  if (fd == -1)
    return;

  /* Check to see if this process is still running.  */
  if (lock->flags & MU_LOCKER_PID)
    {
      char buf[16];
      pid_t pid;
      int nread = read (fd, buf, sizeof (buf) - 1);
      if (nread > 0)
	{
	  buf[nread] = '\0';
	  pid = strtol (buf, NULL, 10);
	  if (pid > 0)
	    {
	      /* Process is gone so we try to remove the lock. */
	      if (kill (pid, 0) == -1)
		stale = 1;
	    }
	  else
	    stale = 1;		/* Corrupted file, remove the lock. */
	}
    }
  /* Check to see if the lock expired.  */
  if (lock->flags & MU_LOCKER_TIME)
    {
      struct stat stbuf;

      fstat (fd, &stbuf);
      /* The lock has expired. */
      if ((time (NULL) - stbuf.st_mtime) > lock->expire_time)
	stale = 1;
    }

  close (fd);
  if (stale)
    unlink (lock->dotlock);
}

static int
stat_check (const char *file, int fd)
{
  struct stat fn_stat;
  struct stat fd_stat;
  int err = 0;

  /* We should always be able to stat a valid fd, so this
     is an error condition. */
  if (lstat (file, &fn_stat) || fstat (fd, &fd_stat))
    err = errno;
  else
    {
      /* If the link and stat don't report the same info, or the
         file is a symlink, fail the locking. */
#define CHK(X) if(X) return EINVAL

      CHK (!S_ISREG (fn_stat.st_mode));
      CHK (!S_ISREG (fd_stat.st_mode));
      CHK (fn_stat.st_nlink != 1);
      CHK (fn_stat.st_dev != fd_stat.st_dev);
      CHK (fn_stat.st_ino != fd_stat.st_ino);
      CHK (fn_stat.st_mode != fd_stat.st_mode);
      CHK (fn_stat.st_nlink != fd_stat.st_nlink);
      CHK (fn_stat.st_uid != fd_stat.st_uid);
      CHK (fn_stat.st_gid != fd_stat.st_gid);
      CHK (fn_stat.st_rdev != fd_stat.st_rdev);

#undef CHK
    }
  return err;
}

int
locker_lock (locker_t lock)
{
  int err = 0;
  int fd = -1;
  int retries = 1;

  if (lock == NULL)
    return EINVAL;

  INVARIANT (lock)
    /* Is the lock already applied? */
    if (lock->refcnt > 0)
    {
      assert (lock->fd != -1);
      lock->refcnt++;
      return 0;
    }

  assert (lock->fd == -1);

  /* Check we are trying to lock a regular file, with a link count
     of 1, that we have permission to read, etc., or don't lock it. */
  if ((fd = open (lock->file, O_RDONLY)) == -1)
    {
      return errno;
    }
  else
    {
      err = stat_check (lock->file, fd);
      close (fd);
      fd = -1;
      if (err)
	{
	  if (err == EINVAL)
	    err = MU_ERR_LOCK_BAD_FILE;
	  return err;
	}
    }

  if (lock->flags & MU_LOCKER_RETRY)
    {
      retries = lock->retries;
    }

  while (retries--)
    {
      /* cleanup after last loop */
      close (fd);
      fd = -1;

      expire_stale_lock (lock);

      /* Try to create the lockfile. */
      fd = open (lock->dotlock, O_WRONLY | O_CREAT | O_EXCL, LOCKFILE_ATTR);
      if (fd == -1)
	{
	  err = errno;

	  /* EEXIST means somebody else has the lock, anything else is an
	     error. */
	  if (err == EEXIST)
	    err = MU_ERR_LOCK_CONFLICT;
	  else
	    return err;
	}
      else
	{
	  err = stat_check (lock->dotlock, fd);
	  if (err == 0)
	    {
	      /* We got the lock! */
	      break;
	    }
	  else
	    {
	      close (fd);
	      unlink (lock->dotlock);

	      /* If there was something invalid about the file/fd, return
	         a more descriptive code. */
	      if (err == EINVAL)
		err = MU_ERR_LOCK_BAD_LOCK;

	      return err;
	    }
	}
      if(retries)
	sleep(lock->retry_sleep);
    }

  if (err)
    return err;

  /* We have the lock. */
  assert (lock->refcnt == 0);

  if (lock->flags & MU_LOCKER_PID)
    {
      char buf[16];
      sprintf (buf, "%ld", (long) getpid ());
      write (fd, buf, strlen (buf));
    }

  lock->refcnt = 1;
  lock->fd = fd;

  INVARIANT (lock);

  return 0;
}

int
locker_touchlock (locker_t lock)
{
  if (!lock)
    return MU_ERR_LOCKER_NULL;

  assert(lock->dotlock);

  INVARIANT(lock);

  if(lock->refcnt > 0)
    return utime (lock->dotlock, NULL);

  return MU_ERR_LOCK_NOT_HELD;
}

int
locker_unlock (locker_t lock)
{
  assert(lock->refcnt >= 0);

  /* FIXME: distinguish between bad args and lock not held */
  if (!lock)
    return MU_ERR_LOCKER_NULL;

  INVARIANT(lock);

  if(lock->refcnt == 0)
    return MU_ERR_LOCK_NOT_HELD;

  assert(lock->fd != -1);

  if (--lock->refcnt > 0)
    return 0;

  assert(lock->refcnt == 0);

  close (lock->fd);
  lock->fd = -1;
  unlink (lock->dotlock);

  INVARIANT(lock);

  return 0;
}
int
locker_remove_lock (locker_t lock)
{
  int err;

  if (!lock)
    return MU_ERR_LOCKER_NULL;

  INVARIANT(lock);

  /* If we hold the lock, do an unlock... */
  if(lock->refcnt > 0)
  {
    /* Force the reference count to 1 to unlock the file. */
    lock->refcnt = 1;
    return locker_unlock(lock);
  }

  /* ... if we don't, unlink the lockfile. */
  err = unlink (lock->dotlock);

  if(err == -1)
  {
    err = errno;
    
    if(err == ENOENT)
      err = MU_ERR_LOCK_NOT_HELD;
  }

  INVARIANT(lock);

  return err;
}

