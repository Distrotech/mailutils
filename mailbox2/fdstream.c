/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU Library General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <mailutils/sys/fdstream.h>
#include <mailutils/monitor.h>
#include <mailutils/error.h>

static void
_fds_cleanup (void *arg)
{
  struct _fds *fds = arg;
  mu_refcount_unlock (fds->refcount);
}

static int
_fds_ref (stream_t stream)
{
  struct _fds *fds = (struct _fds *)stream;
  return mu_refcount_inc (fds->refcount);
}

static void
_fds_destroy (stream_t *pstream)
{
  struct _fds *fds = (struct _fds *)*pstream;
  if (mu_refcount_dec (fds->refcount) == 0)
    {
      mu_refcount_destroy (&fds->refcount);
      free (fds);
    }
}

static int
_fds_close0 (stream_t stream)
{
  struct _fds *fds = (struct _fds *)stream;
  if (fds->fd != -1)
    close (fds->fd);
  fds->fd = -1;
  return 0;
}

static int
_fds_close (stream_t stream)
{
  struct _fds *fds = (struct _fds *)stream;

  mu_refcount_lock (fds->refcount);
  monitor_cleanup_push (_fds_cleanup, fds);
  _fds_close0 (stream);
  mu_refcount_unlock (fds->refcount);
  monitor_cleanup_pop (0);
  return 0;
}

static int
_fds_open (stream_t stream, const char *name, int port, int flags)
{
  (void)stream; (void)name; (void)port; (void)flags;
  return MU_ERROR_NOT_SUPPORTED;
}

static int
_fds_get_fd (stream_t stream, int *fd)
{
  struct _fds *fds = (struct _fds *)stream;

  if (fd == NULL || fds->fd == -1)
    return MU_ERROR_INVALID_PARAMETER;

  *fd = fds->fd;
  return 0;
}

static int
_fds_read (stream_t stream, void *buf, size_t buf_size, size_t *br)
{
  struct _fds *fds = (struct _fds *)stream;
  int bytes = 0;
  int status = 0;

  bytes = read (fds->fd, buf, buf_size);
  if (bytes == -1)
    {
      bytes = 0;
      status = errno;
    }
  if (br)
    *br = bytes;
  return status;
}

static int
_fds_readline (stream_t stream, char *buf, size_t buf_size, size_t *br)
{
  struct _fds *fds = (struct _fds *)stream;
  int status = 0;
  size_t n;
  int nr = 0;
  char c;

  /* Grossly inefficient hopefully they override this */
  for (n = 1; n < buf_size; n++)
    {
      nr = read (fds->fd, &c, 1);
      if (nr == -1) /* Error.  */
	{
	  status = errno;
	  break;
	}
      else if (nr == 1)
	{
	  *buf++ = c;
	  if (c == '\n') /* Newline is stored like fgets().  */
	    break;
	}
      else if (nr == 0)
	{
	  if (n == 1) /* EOF, no data read.  */
	    n = 0;
	  break; /* EOF, some data was read.  */
	}
    }
  *buf = '\0';
  if (br)
    *br = (n == buf_size) ? n - 1: n;
  return status;
}

static int
_fds_write (stream_t stream, const void *buf, size_t buf_size, size_t *bw)
{
  struct _fds *fds = (struct _fds *)stream;
  int bytes = 0;
  int status = 0;

  bytes = write (fds->fd, buf, buf_size);
  if (bytes == -1)
    {
      bytes = 0;
      status = errno;
    }
  if (bw)
    *bw = bytes;
  return status;
}

static int
_fds_seek (stream_t stream, off_t off, enum stream_whence whence)
{
  struct _fds *fds = (struct _fds *)stream;
  int err = 0;
  if (fds->fd)
    {
      if (whence == MU_STREAM_WHENCE_SET)
        off = lseek (fds->fd, off, SEEK_SET);
      else if (whence == MU_STREAM_WHENCE_CUR)
        off = lseek (fds->fd, off, SEEK_CUR);
      else if (whence == MU_STREAM_WHENCE_END)
        off = lseek (fds->fd, off, SEEK_END);
      else
        err = MU_ERROR_INVALID_PARAMETER;
      if (err == -1)
        err = errno;
    }
  return err;
}

static int
_fds_tell (stream_t stream, off_t *off)
{
  struct _fds *fds = (struct _fds *)stream;
  int err = 0;
  if (off)
    {
      *off = lseek (fds->fd, 0, SEEK_CUR);
      if (*off == -1)
	{
	  err = errno;
	  *off = 0;
	}
    }
  return err;;
}

static int
_fds_get_size (stream_t stream, off_t *psize)
{
  struct _fds *fds = (struct _fds *)stream;
  struct stat stbuf;
  int err = 0;
  stbuf.st_size = 0;
  if (fstat (fds->fd,  &stbuf) == -1)
    err = errno;
  if (psize)
    *psize = stbuf.st_size;
  return err;
}

static int
_fds_truncate (stream_t stream, off_t len)
{
  struct _fds *fds = (struct _fds *)stream;
  int err = 0;
  if (ftruncate (fds->fd, len) == -1)
    err = errno ;
  return err;
}

static int
_fds_flush (stream_t stream)
{
  (void)stream;
  return 0;
}

static int
_fds_get_flags (stream_t stream, int *flags)
{
  struct _fds *fds = (struct _fds *)stream;
  if (flags == NULL)
    return MU_ERROR_INVALID_PARAMETER;
  *flags = fds->flags;
  return 0;
}

static int
_fds_get_state (stream_t stream, enum stream_state *state)
{
  (void)stream;
  if (state == NULL)
    return MU_ERROR_INVALID_PARAMETER;
  *state = MU_STREAM_NO_STATE;
  return 0;
}

static int
_fds_is_readready (stream_t stream, int timeout)
{
  struct _fds *fds = (struct _fds *)stream;
  int ready = 0;

  if (fds->fd >= 0)
    {
      struct timeval tv;
      fd_set fset;

      FD_ZERO (&fset);
      FD_SET (fds->fd, &fset);

      tv.tv_sec  = timeout / 100;
      tv.tv_usec = (timeout % 1000) * 1000;

      ready = select (fds->fd + 1, &fset, NULL, NULL,
		      (timeout == -1) ? NULL: &tv);
      ready = (ready == -1) ? 0 : 1;
    }
  return ready;
}

static int
_fds_is_writeready (stream_t stream, int timeout)
{
  struct _fds *fds = (struct _fds *)stream;
  int ready = 0;

  if (fds->fd)
    {
      struct timeval tv;
      fd_set fset;

      FD_ZERO (&fset);
      FD_SET (fds->fd, &fset);

      tv.tv_sec  = timeout / 100;
      tv.tv_usec = (timeout % 1000) * 1000;

      ready = select (fds->fd + 1, NULL, &fset, NULL,
		      (timeout == -1) ? NULL: &tv);
      ready = (ready == -1) ? 0 : 1;
    }
  return ready;
}

static int
_fds_is_exceptionpending (stream_t stream, int timeout)
{
  struct _fds *fds = (struct _fds *)stream;
  int ready = 0;

  if (fds->fd)
    {
      struct timeval tv;
      fd_set fset;

      FD_ZERO (&fset);
      FD_SET  (fds->fd, &fset);

      tv.tv_sec  = timeout / 100;
      tv.tv_usec = (timeout % 1000) * 1000;

      ready = select (fds->fd + 1, NULL, NULL, &fset,
		      (timeout == -1) ? NULL: &tv);
      ready = (ready == -1) ? 0 : 1;
    }
  return 0;
}

static int
_fds_is_open (stream_t stream)
{
  struct _fds *fds = (struct _fds *)stream;
  return fds->fd >= 0;
}

static struct _stream_vtable _fds_vtable =
{
  _fds_ref,
  _fds_destroy,

  _fds_open,
  _fds_close,

  _fds_read,
  _fds_readline,
  _fds_write,

  _fds_seek,
  _fds_tell,

  _fds_get_size,
  _fds_truncate,
  _fds_flush,

  _fds_get_fd,
  _fds_get_flags,
  _fds_get_state,

  _fds_is_readready,
  _fds_is_writeready,
  _fds_is_exceptionpending,

  _fds_is_open
};

int
stream_fd_create (stream_t *pstream, int fd)
{
  struct _fds *fds;

  if (pstream == NULL || fd < 0)
    return MU_ERROR_INVALID_PARAMETER;

  fds = calloc (1, sizeof *fds);
  if (fds == NULL)
    return MU_ERROR_NO_MEMORY;

  mu_refcount_create (&fds->refcount);
  if (fds->refcount == NULL)
    {
      free (fds);
      return MU_ERROR_NO_MEMORY;
    }
  fds->fd = fd;
  fds->base.vtable = &_fds_vtable;
  *pstream = &fds->base;
  return 0;
}
