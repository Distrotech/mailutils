/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000 Free Software Foundation, Inc.

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

#include <locker.h>
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

#define LOCKFILE_ATTR           0444
#define LOCK_EXPIRE_TIME        (5 * 60)

/*
 * Waiting for Brian E. to implement this.
 */

struct _locker
{
  int fd;
  char *fname;
  int locked;
  int flags;
};

int
locker_init (locker_t *plocker, char *filename, size_t len, int flags)
{
  locker_t l;

  if (plocker == NULL)
    return EINVAL;

  if (filename == NULL || len == 0)
    return EINVAL;

  l = malloc (sizeof(*l));
  if (l == NULL)
    return ENOMEM;

  l->fname = calloc (len + 5 + 1, sizeof(char));
  if (l->fname == NULL)
    {
      free (l);
      return ENOMEM;
    }
  memcpy (l->fname, filename, len);
  strcat (l->fname, ".lock");

  l->locked = 0;
  if (flags)
    l->flags = flags;
  else
    l->flags = MU_LOCKER_TIME;
  l->fd = -1;
  *plocker = l;
  return 0;
}

void
locker_destroy (locker_t *plocker)
{
  if (plocker && *plocker)
    {
      free ((*plocker)->fname);
      free (*plocker);
      *plocker = NULL;
    }
}

int
locker_lock (locker_t lock, int flags)
{
  int   fd = -1;
  char  buf[16];
  pid_t pid;
  int   remove = 0;

  (void)flags;
  if (lock == NULL)
    return EINVAL;

  /* check for lock existance
   * if it exists but the process is gone the lock can be removed
   */
  if ((fd = open(lock->fname, O_RDONLY)) != -1)
    {
      if (lock->flags & MU_LOCKER_PID)
        {
          if (read(fd, buf, sizeof (pid_t)) > 0)
            {
              /* check to see if this process is still running */
              if ((pid = atoi(buf)) > 0)
                {
                  /* process is gone so we try to remove the lock */
                  if (kill(pid, 0) == -1)
                    remove = 1;
                }
            }
        }
      if (lock->flags & MU_LOCKER_TIME)
        {
          struct stat buf;

          fstat(fd, &buf);
          /* the lock has expired */
          if ((time(NULL) - buf.st_mtime) > LOCK_EXPIRE_TIME)
            remove = 1;
        }

      close(fd);
      if (remove)
        unlink(lock->fname);
    }

  /* try to create the lockfile */
  if ((fd = open(lock->fname,
		 O_WRONLY | O_CREAT | O_EXCL, LOCKFILE_ATTR)) == -1)
    return (-1);
  /* success */
  sprintf(buf, "%d", getpid());
  write(fd, buf, strlen(buf));

  /* try to get a file lock */
  if (lock->flags & MU_LOCKER_FCNTL)
    {
      struct flock fl;

      memset(&fl, 0, sizeof(struct flock));
      fl.l_type = F_WRLCK;
      if (fcntl(fd, F_SETLK, &fl) == -1)
        {
          /* could not get the file lock */
          close (fd);
          unlink(lock->fname); /* remove the file I created */
          return -1;
        }
    }

  lock->fd = fd;
  return 0;
}

int
locker_touchlock (locker_t lock)
{
 if (!lock || lock->fname || (lock->fd == -1))
    return EINVAL;
  return (utime(lock->fname, NULL));
}

int
locker_unlock (locker_t lock)
{
  if (!lock || !lock->fname || (lock->fd == -1))
    return EINVAL;

  if (lock->flags & MU_LOCKER_FCNTL)
    {
      struct flock fl;

      memset(&fl, 0, sizeof(struct flock));
      fl.l_type = F_UNLCK;
      /* unlock failed  ? */
      if (fcntl(lock->fd, F_SETLK, &fl) == -1)
          return errno;
    }
  lock->locked = 0;
  close(lock->fd);
  lock->fd = -1;
  unlink(lock->fname);
  return 0;
}
