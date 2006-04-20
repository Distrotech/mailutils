/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2005, 2006 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library; if not, write to the
   Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301 USA */

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
#include <fcntl.h>
#include <utime.h>

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <mailutils/errno.h>
#include <mailutils/locker.h>
#include <mailutils/mutil.h>

#define LOCKFILE_ATTR           0644

/* First draft by Brian Edmond. */
/* For subsequent modifications, see the GNU mailutils ChangeLog. */

struct _mu_locker
{
  int refcnt;

  char *file;
  int flags;
  int expire_time;
  int retries;
  int retry_sleep;

  union lock_data {
    struct {
      char *dotlock;
      char *nfslock;
    } dot;
    
    struct {
      char *name;
    } external;
    
    struct {
      int fd;
    } kernel;
  } data;
};

/* Assert that we're managing the refcnt and fd correctly, either
 * we have a lock, and the fd is valid, or refcnt is 0 and fd is -1.
 * And refcnt can never be less than 0.
 */
#define INVARIANT(l) assert((l)->refcnt >= 0);

static void expire_stale_lock (mu_locker_t lock);
static int stat_check (const char *file, int fd, int links);
static int check_file_permissions (const char *file);
static int lock_external (mu_locker_t l, int lock); 
static int _locker_lock_dotlock (mu_locker_t lock);
static int _locker_unlock_dotlock (mu_locker_t lock);
static int _locker_lock_kernel (mu_locker_t lock); 
static int _locker_unlock_kernel (mu_locker_t lock);

static int mu_locker_default_flags = MU_LOCKER_DEFAULT;
static time_t mu_locker_retry_timeout = MU_LOCKER_RETRY_SLEEP;
static size_t mu_locker_retry_count = MU_LOCKER_RETRIES;
static time_t mu_locker_expire_timeout = MU_LOCKER_EXPIRE_TIME;
static char *mu_locker_external_program = NULL;

int
mu_locker_set_default_flags (int flags, enum mu_locker_set_mode mode)
{
  switch (mode)
    {
    case mu_locker_assign:
      mu_locker_default_flags = flags;
      break;

    case mu_locker_set_bit:
      mu_locker_default_flags |= flags;
      break;

    case mu_locker_clear_bit:
      mu_locker_default_flags &= ~flags;
      break;

    default:
      return EINVAL;
    }
  return 0;
}

void
mu_locker_set_default_retry_timeout (time_t to)
{
  mu_locker_retry_timeout = to;
}

void
mu_locker_set_default_retry_count (size_t n)
{
  mu_locker_retry_count = n;
}

void
mu_locker_set_default_expire_timeout (time_t t)
{
  mu_locker_expire_timeout = t;
}

void
mu_locker_set_default_external_program (char *path)
{
  free (mu_locker_external_program);
  mu_locker_external_program = strdup (path);
}

int
mu_locker_create (mu_locker_t *plocker, const char *filename_, int flags)
{
  mu_locker_t l;
  char filename[_POSIX_PATH_MAX];
  int err = 0;

  if (plocker == NULL)
    return MU_ERR_OUT_PTR_NULL;

  if (filename_ == NULL)
    return EINVAL;

  if((err = mu_unroll_symlink(filename, sizeof(filename), filename_)))
    return err;

  l = calloc (1, sizeof (*l));

  if (l == NULL)
    return ENOMEM;

  l->file = strdup(filename);

  if (l->file == NULL)
    {
      free (l);
      return ENOMEM;
    }

  if (strcmp (filename, "/dev/null") == 0)
    l->flags = MU_LOCKER_NULL;
  else if (flags)
    l->flags = flags;
  else
    l->flags = mu_locker_default_flags;

  l->expire_time = mu_locker_expire_timeout;
  l->retries = mu_locker_retry_count;
  l->retry_sleep = mu_locker_retry_timeout;

  /* Initialize locker-type-specific data */
  if (l->flags & MU_LOCKER_EXTERNAL)
    {
      if (!(l->data.external.name = strdup (mu_locker_external_program ?
					    mu_locker_external_program :
					    MU_LOCKER_EXTERNAL_PROGRAM)))
	{
	  mu_locker_destroy (&l);
	  return ENOMEM;
	}
    }
  else if (!(l->flags & MU_LOCKER_KERNEL))
    {
      l->data.dot.dotlock = malloc (strlen (l->file)
				    + 5 /*strlen(".lock")*/ + 1);

      if (!l->data.dot.dotlock)
	{
	  free (l->file);
	  free (l);
	  return ENOMEM;
	}

      sprintf (l->data.dot.dotlock, "%s.lock", l->file);
    }

  INVARIANT(l);

  *plocker = l;

  return 0;
}

void
_locker_destroy_private (mu_locker_t locker)
{
  if (locker)
    {
      if (locker->flags & MU_LOCKER_EXTERNAL)
	free (locker->data.external.name);
      else if (locker->flags & MU_LOCKER_KERNEL)
	/* nothing */;
      else
	{
	  free (locker->data.dot.dotlock);
	  locker->data.dot.dotlock = NULL;
	  free (locker->data.dot.nfslock);
	  locker->data.dot.nfslock = NULL;
	}
    }
}

void
mu_locker_destroy (mu_locker_t *plocker)
{
  if (plocker && *plocker)
    {
      _locker_destroy_private (*plocker);
      free ((*plocker)->file);
      free (*plocker);
      *plocker = NULL;
    }
}

int
mu_locker_set_flags (mu_locker_t locker, int flags)
{
  if (!locker)
    return MU_ERR_LOCKER_NULL;

  locker->flags = flags;

  return 0;
}

int
mu_locker_set_expire_time (mu_locker_t locker, int etime)
{
  if (!locker)
    return MU_ERR_LOCKER_NULL;

  if (etime <= 0)
    return EINVAL;

  locker->expire_time = etime;

  return 0;
}

int
mu_locker_set_retries (mu_locker_t locker, int retries)
{
  if (!locker)
    return MU_ERR_LOCKER_NULL;

  if (retries <= 0)
    return EINVAL;

  locker->retries = retries;

  return 0;
}

int
mu_locker_set_retry_sleep (mu_locker_t locker, int retry_sleep)
{
  if (!locker)
    return MU_ERR_LOCKER_NULL;

  if (retry_sleep <= 0)
    return EINVAL;

  locker->retry_sleep = retry_sleep;

  return 0;
}

int
mu_locker_set_external (mu_locker_t locker, const char* program)
{
  char* p = NULL;

  if (!locker)
    return MU_ERR_LOCKER_NULL;
  if (!(locker->flags & MU_LOCKER_EXTERNAL))
    return EINVAL;
    
  /* program can be NULL */
  if (program != 0)
    {
      p = strdup (program);
      if (!p)
	return ENOMEM;
  }

  free (locker->data.external.name);
  locker->data.external.name = p;

  return 0;
}

int
mu_locker_get_flags (mu_locker_t locker, int *flags)
{
  if (!locker)
    return MU_ERR_LOCKER_NULL;

  if(!flags)
    return EINVAL;

  *flags = locker->flags;

  return 0;
}

int
mu_locker_get_expire_time (mu_locker_t locker, int *ptime)
{
  if (!locker)
    return MU_ERR_LOCKER_NULL;

  if (!ptime)
    return EINVAL;

  *ptime = locker->expire_time;

  return 0;
}

int
mu_locker_get_retries (mu_locker_t locker, int *retries)
{
  if (!locker)
    return MU_ERR_LOCKER_NULL;

  if (!retries)
    return EINVAL;

  *retries = locker->retries;

  return 0;
}

int
mu_locker_get_retry_sleep (mu_locker_t locker, int *retry_sleep)
{
  if (!locker)
    return MU_ERR_LOCKER_NULL;

  if (!retry_sleep)
    return EINVAL;

  *retry_sleep = locker->retry_sleep;

  return 0;
}

int
mu_locker_lock (mu_locker_t lock)
{
  int rc;
  int retries = 1;

  if (lock == NULL)
    return EINVAL;

  if (lock->flags == MU_LOCKER_NULL)
    return 0;
  
  INVARIANT (lock)
    /* Is the lock already applied? */
  if (lock->refcnt > 0)
    {
      lock->refcnt++;
      return 0;
    }

  if (access (lock->file, W_OK))
    {
      /* There is no use trying to lock the file if we are not
	 allowed to write to it */
      _locker_destroy_private (lock);
      lock->flags |= MU_LOCKER_NULL;
      return 0;
    }
  
  /* Check we are trying to lock a regular file, with a link count
     of 1, that we have permission to read, etc., or don't lock it. */
  if ((rc = check_file_permissions(lock->file)))
      return rc;

  /* Do the lock with an external program, if requested. */
  if (lock->flags & MU_LOCKER_EXTERNAL)
    return lock_external (lock, 1);
  else if (!(lock->flags & MU_LOCKER_KERNEL))
    {
      char *tmp, *p;

      tmp = strdup (lock->file);
      if (!tmp)
	return ENOMEM;

      strcpy (tmp, lock->file);
      p = strrchr (tmp, '/');
      if (!p)
	{
	  free (tmp);
	  tmp = strdup (".");
	  if (!tmp)
	    return ENOMEM;
	}
      else
	*p = 0; 

      if (access (tmp, W_OK))
	{
	  /* Fallback to kernel locking */
	  _locker_destroy_private (lock);
	  lock->flags |= MU_LOCKER_KERNEL;
	}
      free (tmp);
    }

  if (lock->flags & MU_LOCKER_RETRY)
    {
      retries = lock->retries;
    }

  while (retries--)
    {
      if (lock->flags & MU_LOCKER_KERNEL)
	rc = _locker_lock_kernel (lock);
      else
	rc = _locker_lock_dotlock (lock);
      if (rc == EAGAIN && retries)
	sleep (lock->retry_sleep);
      else if (rc)
	return rc;
      else /* rc == 0 */
	break;
    }

  lock->refcnt = 1;

  return 0;
}

int
mu_locker_touchlock (mu_locker_t lock)
{
  if (!lock)
    return MU_ERR_LOCKER_NULL;

  if (lock->flags == MU_LOCKER_NULL)
    return 0;
  
  if (lock->refcnt > 0)
    return utime (lock->data.dot.dotlock, NULL);

  return MU_ERR_LOCK_NOT_HELD;
}

int
mu_locker_unlock (mu_locker_t lock)
{
  int rc = 0;

  if (!lock)
    return MU_ERR_LOCKER_NULL;

  if (lock->flags == MU_LOCKER_NULL)
    return 0;

  if (lock->refcnt == 0)
    return MU_ERR_LOCK_NOT_HELD;

  /* Do the lock with an external program, if requested. */
  if (lock->flags & MU_LOCKER_EXTERNAL)
    return lock_external (lock, 0);
  
  if (lock->refcnt > 1)
    {
      lock->refcnt--;
      return 0;
    }

  if ((rc = check_file_permissions (lock->file)))
    return rc;

  if (lock->flags & MU_LOCKER_KERNEL)
    rc = _locker_unlock_kernel (lock);
  else
    rc = _locker_unlock_dotlock (lock);

  if (rc == 0)
    lock->refcnt = 0;

  return rc;
}

int
mu_locker_remove_lock (mu_locker_t lock)
{
  int err;

  if (!lock)
    return MU_ERR_LOCKER_NULL;

  if (lock->flags == MU_LOCKER_NULL)
    return 0;

  /* Force the reference count to 1 to unlock the file. */
  lock->refcnt = 1;
  err = mu_locker_unlock (lock);

  return err;
}

/* expire a stale lock (if MU_LOCKER_PID or MU_LOCKER_TIME) */
static void
expire_stale_lock (mu_locker_t lock)
{
  int stale = 0;
  int fd = open (lock->data.dot.dotlock, O_RDONLY);
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
    unlink (lock->data.dot.dotlock);
}

static int
stat_check (const char *file, int fd, int links)
{
  struct stat fn_stat;
  struct stat fd_stat;
  int err = 0;
  int localfd = -1;

  if (fd == -1)
    {
      localfd = open(file, O_RDONLY);
      
      if (localfd == -1)
	return errno;
      fd = localfd;
    }

  /* We should always be able to stat a valid fd, so this
     is an error condition. */
  if (lstat (file, &fn_stat) || fstat (fd, &fd_stat))
    err = errno;
  else
    {
      /* If the link and stat don't report the same info, or the
         file is a symlink, fail the locking. */
#define CHK(X) if(X) err = EINVAL

      CHK (!S_ISREG (fn_stat.st_mode));
      CHK (!S_ISREG (fd_stat.st_mode));
      CHK (fn_stat.st_nlink != links);
      CHK (fn_stat.st_dev != fd_stat.st_dev);
      CHK (fn_stat.st_ino != fd_stat.st_ino);
      CHK (fn_stat.st_mode != fd_stat.st_mode);
      CHK (fn_stat.st_nlink != fd_stat.st_nlink);
      CHK (fn_stat.st_uid != fd_stat.st_uid);
      CHK (fn_stat.st_gid != fd_stat.st_gid);
      CHK (fn_stat.st_rdev != fd_stat.st_rdev);

#undef CHK
    }
  if (localfd != -1)
    close (localfd);

  return err;
}

static int
check_file_permissions (const char *file)
{
  int fd = -1;
  int err = 0;

  if ((fd = open (file, O_RDONLY)) == -1)
    return errno == ENOENT ? 0 : errno;

  err = stat_check (file, fd, 1);
  close (fd);
  fd = -1;
  if (err)
    {
      if (err == EINVAL)
	err = MU_ERR_LOCK_BAD_FILE;
      return err;
    }

  return 0;
}

#ifndef MAXHOSTNAMELEN
# define MAXHOSTNAMELEN 256
#endif

/* Locker-specific lock/unlock functions */
int
_locker_lock_dotlock (mu_locker_t lock)
{
  char host[MAXHOSTNAMELEN + 1] = "localhost";
  char pid[11];		/* 10 is strlen(2^32 = 4294967296) */
  char now[11];
  size_t sz = 0;
  int err = 0;
  int fd;
    
  if (lock->data.dot.nfslock)
    {
      unlink (lock->data.dot.nfslock);
      free (lock->data.dot.nfslock);
      lock->data.dot.nfslock = 0;
    }

  expire_stale_lock (lock);

  /* build the NFS hitching-post to the lock file */

  gethostname (host, sizeof (host));
  host[MAXHOSTNAMELEN] = 0;

  snprintf (now, sizeof (now), "%lu", (unsigned long) time (0));
  now[sizeof (now) - 1] = 0;

  snprintf (pid, sizeof (pid), "%lu", (unsigned long) getpid ());
  pid[sizeof (pid) - 1] = 0;
		  
  sz = strlen (lock->file) + 1 /* "." */
    + strlen (pid) + 1 /* "." */
    + strlen (now) + 1 /* "." */
    + strlen (host) + 1;
  
  lock->data.dot.nfslock = malloc (sz);
  
  if (!lock->data.dot.nfslock)
    return ENOMEM;
  
  snprintf (lock->data.dot.nfslock, sz, "%s.%s.%s.%s", lock->file, pid, now,
	    host);
  
  fd = open (lock->data.dot.nfslock,
	     O_WRONLY | O_CREAT | O_EXCL, LOCKFILE_ATTR);
  if (fd == -1)
    {
      if (errno == EEXIST)
	return EAGAIN;
      else
	return errno;
    }
  close (fd);
  
  /* Try to link to the lockfile. */
  if (link (lock->data.dot.nfslock, lock->data.dot.dotlock) == -1)
    {
      unlink (lock->data.dot.nfslock);
      if (errno == EEXIST)
	return MU_ERR_LOCK_CONFLICT;
      return errno;
    }

  if ((fd = open (lock->data.dot.dotlock, O_RDWR)) == -1)
    {
      unlink (lock->data.dot.nfslock);
      return errno;
    }
  
  err = stat_check (lock->data.dot.nfslock, fd, 2);
  if (err)
    {
      unlink (lock->data.dot.nfslock);
      if (err == EINVAL)
	return MU_ERR_LOCK_BAD_LOCK;
      return errno;
    }

  unlink (lock->data.dot.nfslock);

  /* If no errors, we have the lock. */
  assert (lock->refcnt == 0);

  if (lock->flags & MU_LOCKER_PID)
    {
      char buf[16];
      sprintf (buf, "%ld", (long) getpid ());
      write (fd, buf, strlen (buf));
    }
  close(fd);
  return 0;
}  

int
_locker_unlock_dotlock (mu_locker_t lock)
{
  if (unlink (lock->data.dot.dotlock) == -1)
    {
      int err = errno;
      if (err == ENOENT)
	{
	  lock->refcnt = 0;
	  err = MU_ERR_LOCK_NOT_HELD;
	  return err;
	}
      return err;
    }
  return 0;
}

int
_locker_lock_kernel (mu_locker_t lock)
{
  int fd;
  struct flock fl;

  fd = open (lock->file, O_RDWR);
  if (fd == -1)
    return errno;
  lock->data.kernel.fd = fd;
  fl.l_type = F_WRLCK;
  fl.l_whence = SEEK_SET;
  fl.l_start = 0;
  fl.l_len = 0; /* Lock entire file */
  if (fcntl (fd, F_SETLK, &fl))
    {
#ifdef EACCESS      
      if (errno == EACCESS)
	return EAGAIN;
#endif
      if (errno == EAGAIN)
	return EAGAIN;
      return errno;
    }
  return 0;
}

int
_locker_unlock_kernel (mu_locker_t lock)
{
  struct flock fl;

  fl.l_type = F_UNLCK;
  fl.l_whence = SEEK_SET;
  fl.l_start = 0;
  fl.l_len = 0; /* Unlock entire file */
  if (fcntl (lock->data.kernel.fd, F_SETLK, &fl))
    {
#ifdef EACCESS
      if (errno == EACCESS)
	return EAGAIN;
#endif
      if (errno == EAGAIN)
	return EAGAIN;
      return errno;
    }
  close (lock->data.kernel.fd);
  return 0;
}

/*
  Estimate 1 decimal digit per 3 bits, + 1 for round off.
*/
#define DEC_DIGS_PER_INT (sizeof(int) * 8 / 3 + 1)

static int
lock_external (mu_locker_t l, int lock)
{
  int err = 0;
  char *av[6];
  int ac = 0;
  char aforce[3 + DEC_DIGS_PER_INT + 1];
  char aretry[3 + DEC_DIGS_PER_INT + 1];
  int status = 0;

  assert (l);
  assert (l->flags & MU_LOCKER_EXTERNAL);
  assert (lock == !l->refcnt);
  /* lock is true, refcnt is 0 or lock is false and refcnt is 1 */

  av[ac++] = l->data.external.name ?
                   l->data.external.name : MU_LOCKER_EXTERNAL_PROGRAM;

  if (l->flags & MU_LOCKER_TIME)
    {
      snprintf (aforce, sizeof (aforce), "-f%d", l->expire_time);
      aforce[sizeof (aforce) - 1] = 0;
      av[ac++] = aforce;
    }
  
  if (l->flags & MU_LOCKER_RETRY)
    {
      snprintf (aretry, sizeof (aretry), "-r%d", l->retries);
      aretry[sizeof (aretry) - 1] = 0;
      av[ac++] = aretry;
    }

  if (lock == 0)
    {
      av[ac++] = "-u";
    }

  av[ac++] = l->file;

  av[ac++] = NULL;

  if ((err = mu_spawnvp (av[0], av, &status)))
    return err;

  if (!WIFEXITED (status))
    {
      err = MU_ERR_LOCK_EXT_KILLED;
    }
  else
    {
      switch (WEXITSTATUS (status))
	{
	case 127:
	  err = MU_ERR_LOCK_EXT_FAIL;
	  break;
	case MU_DL_EX_OK:
	  err = 0;
	  l->refcnt = lock;
	  break;
	case MU_DL_EX_NEXIST:
	  err = MU_ERR_LOCK_NOT_HELD;
	  break;
	case MU_DL_EX_EXIST:
	  err = MU_ERR_LOCK_CONFLICT;
	  break;
	case MU_DL_EX_PERM:
	  err = EPERM;
	  break;
	default:
	case MU_DL_EX_ERROR:
	  err = MU_ERR_LOCK_EXT_ERR;
	  break;
	}
    }

  return err;
}

