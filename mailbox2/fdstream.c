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
_stream_fd_cleanup (void *arg)
{
  struct _stream_fd *fds = arg;
  mu_refcount_unlock (fds->refcount);
}

int
_stream_fd_ref (stream_t stream)
{
  struct _stream_fd *fds = (struct _stream_fd *)stream;
  return mu_refcount_inc (fds->refcount);
}

void
_stream_fd_destroy (stream_t *pstream)
{
  struct _stream_fd *fds = (struct _stream_fd *)*pstream;
  if (mu_refcount_dec (fds->refcount) == 0)
    {
      mu_refcount_destroy (&fds->refcount);
      free (fds);
    }
}

static int
_stream_fd_close0 (stream_t stream)
{
  struct _stream_fd *fds = (struct _stream_fd *)stream;
  fds->state = MU_STREAM_STATE_CLOSE;
  if (fds->fd != -1)
    close (fds->fd);
  fds->fd = -1;
  return 0;
}

int
_stream_fd_close (stream_t stream)
{
  struct _stream_fd *fds = (struct _stream_fd *)stream;

  mu_refcount_lock (fds->refcount);
  monitor_cleanup_push (_stream_fd_cleanup, fds);
  _stream_fd_close0 (stream);
  mu_refcount_unlock (fds->refcount);
  monitor_cleanup_pop (0);
  return 0;
}

int
_stream_fd_open (stream_t stream, const char *name, int port, int flags)
{
  (void)stream; (void)name; (void)port; (void)flags;
  return MU_ERROR_NOT_SUPPORTED;
}

int
_stream_fd_get_fd (stream_t stream, int *fd)
{
  struct _stream_fd *fds = (struct _stream_fd *)stream;

  if (fd == NULL || fds->fd == -1)
    return MU_ERROR_INVALID_PARAMETER;

  *fd = fds->fd;
  return 0;
}

int
_stream_fd_read (stream_t stream, void *buf, size_t buf_size, size_t *br)
{
  struct _stream_fd *fds = (struct _stream_fd *)stream;
  int bytes = 0;
  int status = 0;

  fds->state = MU_STREAM_STATE_READ;
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

int
_stream_fd_readline (stream_t stream, char *buf, size_t buf_size, size_t *br)
{
  struct _stream_fd *fds = (struct _stream_fd *)stream;
  int status = 0;
  size_t n;
  int nr = 0;
  char c;

  fds->state = MU_STREAM_STATE_READ;
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

int
_stream_fd_write (stream_t stream, const void *buf, size_t buf_size, size_t *bw)
{
  struct _stream_fd *fds = (struct _stream_fd *)stream;
  int bytes = 0;
  int status = 0;

  fds->state = MU_STREAM_STATE_WRITE;
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

int
_stream_fd_seek (stream_t stream, off_t off, enum stream_whence whence)
{
  struct _stream_fd *fds = (struct _stream_fd *)stream;
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

int
_stream_fd_tell (stream_t stream, off_t *off)
{
  struct _stream_fd *fds = (struct _stream_fd *)stream;
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
  return err;
}

int
_stream_fd_get_size (stream_t stream, off_t *psize)
{
  struct _stream_fd *fds = (struct _stream_fd *)stream;
  struct stat stbuf;
  int err = 0;
  stbuf.st_size = 0;
  if (fstat (fds->fd,  &stbuf) == -1)
    err = errno;
  if (psize)
    *psize = stbuf.st_size;
  return err;
}

int
_stream_fd_truncate (stream_t stream, off_t len)
{
  struct _stream_fd *fds = (struct _stream_fd *)stream;
  int err = 0;
  if (ftruncate (fds->fd, len) == -1)
    err = errno;
  return err;
}

int
_stream_fd_flush (stream_t stream)
{
  (void)stream;
  return 0;
}

int
_stream_fd_get_flags (stream_t stream, int *flags)
{
  struct _stream_fd *fds = (struct _stream_fd *)stream;
  if (flags == NULL)
    return MU_ERROR_INVALID_PARAMETER;
  *flags = fds->flags;
  return 0;
}

int
_stream_fd_get_state (stream_t stream, enum stream_state *state)
{
  struct _stream_fd *fds = (struct _stream_fd *)stream;
  if (state == NULL)
    return MU_ERROR_INVALID_PARAMETER;
  *state = fds->state;
  return 0;
}

int
_stream_fd_is_readready (stream_t stream, int timeout)
{
  struct _stream_fd *fds = (struct _stream_fd *)stream;
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

int
_stream_fd_is_writeready (stream_t stream, int timeout)
{
  struct _stream_fd *fds = (struct _stream_fd *)stream;
  int ready = 0;

  if (fds->fd >= 0)
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

int
_stream_fd_is_exceptionpending (stream_t stream, int timeout)
{
  struct _stream_fd *fds = (struct _stream_fd *)stream;
  int ready = 0;

  if (fds->fd >= 0)
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

int
_stream_fd_is_open (stream_t stream)
{
  struct _stream_fd *fds = (struct _stream_fd *)stream;
  return fds->fd >= 0;
}

static struct _stream_vtable _stream_fd_vtable =
{
  _stream_fd_ref,
  _stream_fd_destroy,

  _stream_fd_open,
  _stream_fd_close,

  _stream_fd_read,
  _stream_fd_readline,
  _stream_fd_write,

  _stream_fd_seek,
  _stream_fd_tell,

  _stream_fd_get_size,
  _stream_fd_truncate,
  _stream_fd_flush,

  _stream_fd_get_fd,
  _stream_fd_get_flags,
  _stream_fd_get_state,

  _stream_fd_is_readready,
  _stream_fd_is_writeready,
  _stream_fd_is_exceptionpending,

  _stream_fd_is_open
};


int
_stream_fd_ctor (struct _stream_fd *fds, int fd)
{
  mu_refcount_create (&fds->refcount);
  if (fds->refcount == NULL)
    return MU_ERROR_NO_MEMORY;
  fds->fd = fd;
  fds->base.vtable = &_stream_fd_vtable;
  return 0;
}

void
_stream_fd_dtor (struct _stream_fd *fds)
{
  mu_refcount_destroy (&fds->refcount);
}

int
stream_fd_create (stream_t *pstream, int fd)
{
  struct _stream_fd *fds;
  int status;

  if (pstream == NULL || fd < 0)
    return MU_ERROR_INVALID_PARAMETER;

  fds = calloc (1, sizeof *fds);
  if (fds == NULL)
    return MU_ERROR_NO_MEMORY;

  status = _stream_fd_ctor (fds, fd);
  if (status != 0)
    {
      free (fds);
      return status;
    }
  *pstream = &fds->base;
  return 0;
}
