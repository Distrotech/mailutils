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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <mailutils/sys/mstream.h>
#include <mailutils/error.h>

#ifdef _POSIX_MAPPED_FILES
#include <sys/mman.h>

#ifndef MAP_FAILED
# define MAP_FAILED (void*)-1
#endif

static int
_map_add_ref (stream_t stream)
{
  int status;
  struct _ms *ms = (struct _ms *)stream;
  monitor_lock (ms->lock);
  status = ++ms->ref;
  monitor_unlock (ms->lock);
  return status;
}

static int
_map_destroy (stream_t stream)
{
  struct _ms *ms = (struct _ms *)stream;

  if (ms->ptr != MAP_FAILED)
    {
      if (ms->ptr)
	munmap (ms->ptr, ms->size);
      if (ms->fd != -1)
	close (ms->fd);
      monitor_destroy (ms->lock);
    }
  free (ms);
  return 0;
}

static int
_map_release (stream_t stream)
{
  int status;
  struct _ms *ms = (struct _ms *)stream;
  monitor_lock (ms->lock);
  status = --ms->ref;
  if (status <= 0)
    {
      monitor_unlock (ms->lock);
      _map_destroy (stream);
      return 0;
    }
  monitor_unlock (ms->lock);
  return status;
}

static int
_map_read (stream_t stream, void *optr, size_t osize, size_t *nbytes)
{
  struct _ms *ms = (struct _ms *)stream;
  size_t n = 0;

  monitor_lock (ms->lock);
  if (ms->ptr != MAP_FAILED && ms->ptr)
    {
      if (ms->offset < (off_t)ms->size)
	{
	  n = ((ms->offset + osize) > ms->size) ?
	    ms->size - ms->offset :  osize;
	  memcpy (optr, ms->ptr + ms->offset, n);
	  ms->offset += n;
	}
    }
  monitor_unlock (ms->lock);

  if (nbytes)
    *nbytes = n;
  return 0;
}

static int
_map_readline (stream_t stream, char *optr, size_t osize, size_t *nbytes)
{
  struct _ms *ms = (struct _ms *)stream;
  size_t n = 0;

  monitor_lock (ms->lock);
  if (ms->ptr != MAP_FAILED && ms->ptr)
    {
      if (ms->offset < (off_t)ms->size)
	{
	  /* Save space for the null byte.  */
	  char *nl;
	  osize--;
	  nl = memchr (ms->ptr + ms->offset, '\n', ms->size - ms->offset);
	  n = (nl) ? nl - (ms->ptr + ms->offset) + 1 : ms->size - ms->offset;
	  n = (n > osize)  ? osize : n;
	  memcpy (optr, ms->ptr + ms->offset, n);
	  optr[n] = '\0';
	  ms->offset += n;
	}
    }
  monitor_unlock (ms->lock);

  if (nbytes)
    *nbytes = n;
  return 0;
}

static int
_map_write (stream_t stream, const void *iptr, size_t isize, size_t *nbytes)
{
  struct _ms *ms = (struct _ms *)stream;
  int err = 0;
  size_t n = 0;

  monitor_lock (ms->lock);
  if (ms->mflags & PROT_WRITE)
    {
      /* Bigger we have to remmap.  */
      if (ms->size < (ms->offset + isize))
	{
	  if (ms->ptr != MAP_FAILED && munmap (ms->ptr, ms->size) == 0)
	    {
	      ms->ptr = MAP_FAILED;
	      if (ftruncate (ms->fd, ms->offset + isize) == 0)
		{
		  ms->ptr = mmap (0, ms->offset + isize, ms->mflags,
				  MAP_SHARED, ms->fd, 0);
		  if (ms->ptr != MAP_FAILED)
		    ms->size = ms->offset + isize;
		}
	    }
	}

      if (ms->ptr != MAP_FAILED)
	{
	  if (isize > 0)
	    memcpy (ms->ptr + ms->offset, iptr, isize);
	  ms->offset += isize;
	  n = isize;
	}
      else
	err = MU_ERROR_IO;
    }
  else
    err = MU_ERROR_IO;
  monitor_unlock (ms->lock);

  if (nbytes)
    *nbytes = n;
  return err;
}

static int
_map_truncate (stream_t stream, off_t len)
{
  struct _ms *ms = (struct _ms *)stream;
  int err = 0;

  monitor_lock (ms->lock);
  if (ms->ptr != MAP_FAILED)
    {
      /* Remap.  */
      if (ms->ptr && munmap (ms->ptr, ms->size) == 0)
	{
	  if (ftruncate (ms->fd, len) == 0)
	    {
	      ms->ptr = (len) ?
		mmap (0, len, ms->mflags, MAP_SHARED, ms->fd, 0) : NULL;
	      if (ms->ptr != MAP_FAILED)
		{
		  ms->size = len;
		}
	      else
		err = errno;
	    }
	  else
	    err = errno;
	}
     }
  monitor_unlock (ms->lock);
  return err;
}

static int
_map_get_size (stream_t stream, off_t *psize)
{
  struct _ms *ms = (struct _ms *)stream;
  struct stat stbuf;
  int err = 0;

  monitor_lock (ms->lock);
  stbuf.st_size = 0;
  if (ms->ptr != MAP_FAILED)
    {
      if (ms->ptr)
	msync (ms->ptr, ms->size, MS_SYNC);

      if (fstat(ms->fd, &stbuf) == 0)
	{
	  /* Remap.  */
	  if (ms->size != (size_t)stbuf.st_size)
	    {
	      if (ms->ptr && munmap (ms->ptr, ms->size) == 0)
		{
		  if (ms->size)
		    {
		      ms->ptr = mmap (0, ms->size, ms->mflags , MAP_SHARED,
				       ms->fd, 0);
		      if (ms->ptr != MAP_FAILED)
			ms->size = stbuf.st_size;
		      else
			err = errno;
		    }
		  else
		    ms->ptr = NULL;
		}
	      else
		err = errno;
	    }
	}
    }
  monitor_unlock (ms->lock);
  if (psize)
    *psize = stbuf.st_size;
  return err;
}

static int
_map_flush (stream_t stream)
{
  int err = 0;
  struct _ms *ms = (struct _ms *)stream;
  monitor_lock (ms->lock);
  if (ms->ptr != MAP_FAILED && ms->ptr != NULL)
    err = msync (ms->ptr, ms->size, MS_SYNC);
  monitor_unlock (ms->lock);
  return 0;
}

static int
_map_get_fd (stream_t stream, int *pfd)
{
  struct _ms *ms = (struct _ms *)stream;
  if (pfd)
    *pfd = ms->fd;
  return 0;
}

static int
_map_get_flags (stream_t stream, int *flags)
{
  struct _ms *ms = (struct _ms *)stream;
  if (flags == NULL)
    return MU_ERROR_INVALID_PARAMETER;
  *flags = ms->flags;
  return 0;
}

static int
_map_get_state (stream_t stream, enum stream_state *state)
{
  (void)stream;
  if (state == NULL)
    return MU_ERROR_INVALID_PARAMETER;
  *state = MU_STREAM_NO_STATE;
  return 0;
}

static int
_map_seek (stream_t stream, off_t off, enum stream_whence whence)
{
  struct _ms *ms = (struct _ms *)stream;
  off_t noff = ms->offset;
  int err = 0;
  if (whence == MU_STREAM_WHENCE_SET)
      noff = off;
  else if (whence == MU_STREAM_WHENCE_CUR)
    noff += off;
  else if (whence == MU_STREAM_WHENCE_END)
    noff = ms->size + off;
  else
    noff = -1; /* error.  */
  if (noff >= 0)
    {
      if (noff > ms->offset)
	_map_truncate (stream, noff);
      ms->offset = noff;
    }
  else
    err = MU_ERROR_INVALID_PARAMETER;
  return err;
}

static int
_map_tell (stream_t stream, off_t *off)
{
  struct _ms *ms = (struct _ms *)stream;
  if (off == NULL)
    return MU_ERROR_INVALID_PARAMETER;
  *off = ms->offset;
  return 0;
}

static int
_map_close (stream_t stream)
{
  struct _ms *ms = (struct _ms *)stream;
  int err = 0;
  monitor_lock (ms->lock);
  if (ms->ptr != MAP_FAILED)
    {
      if (ms->ptr && munmap (ms->ptr, ms->size) != 0)
	err = errno;
      ms->ptr = MAP_FAILED;
    }
  if (ms->fd != -1)
    if (close (ms->fd) != 0)
      err = errno;
  ms->fd = -1;
  monitor_unlock (ms->lock);
  return err;
}

static int
_map_is_open (stream_t stream)
{
  struct _ms *ms = (struct _ms *)stream;
  return (ms->ptr == MAP_FAILED) ? 0 : 1;
}

static int
_map_is_readready (stream_t stream, int timeout)
{
  (void)timeout;
  return _map_is_open (stream);
}

static int
_map_is_writeready (stream_t stream, int timeout)
{
  (void)timeout;
  return _map_is_open (stream);
}

static int
_map_is_exceptionpending (stream_t stream, int timeout)
{
  (void)stream;
  (void)timeout;
  return 0;
}


static int
_map_open (stream_t stream, const char *filename, int port, int flags)
{
  struct _ms *ms = (struct _ms *)stream;
  int mflag, flg;
  struct stat st;

  (void)port; /* Ignored.  */

  /* Close any previous file.  */
  if (ms->ptr != MAP_FAILED)
    {
      if (ms->ptr)
	munmap (ms->ptr, ms->size);
      ms->ptr = MAP_FAILED;
    }
  if (ms->fd != -1)
    {
      close (ms->fd);
      ms->fd = -1;
    }
  /* Map the flags to the system equivalent */
  if ((flags & MU_STREAM_WRITE) && (flags & MU_STREAM_READ))
    return EINVAL;
  else if (flags & MU_STREAM_WRITE)
    {
      mflag = PROT_WRITE;
      flg = O_WRONLY;
    }
  else if (flags & MU_STREAM_RDWR)
    {
      mflag = PROT_READ | PROT_WRITE;
      flg = O_RDWR;
    }
  else if (flags & MU_STREAM_CREAT)
    return ENOSYS;
  else /* default */
    {
      mflag = PROT_READ;
      flg = O_RDONLY;
    }

  ms->fd = open (filename, flg);
  if (ms->fd < 0)
    return errno;
  if (fstat (ms->fd, &st) != 0)
    {
      int err = errno;
      close (ms->fd);
      return err;
    }
  ms->size = st.st_size;
  if (ms->size)
    {
      ms->ptr = mmap (0, ms->size, mflag , MAP_SHARED, ms->fd, 0);
      if (ms->ptr == MAP_FAILED)
	{
	  int err = errno;
	  close (ms->fd);
	  ms->ptr = MAP_FAILED;
	  return err;
	}
    }
  else
    ms->ptr = NULL;
  ms->mflags = mflag;
  ms->flags = flags;
  return 0;
}

static struct _stream_vtable _map_vtable =
{
  _map_add_ref,
  _map_release,
  _map_destroy,

  _map_open,
  _map_close,

  _map_read,
  _map_readline,
  _map_write,

  _map_seek,
  _map_tell,

  _map_get_size,
  _map_truncate,
  _map_flush,

  _map_get_fd,
  _map_get_flags,
  _map_get_state,

  _map_is_readready,
  _map_is_writeready,
  _map_is_exceptionpending,

  _map_is_open
};

#endif /* _POSIX_MAPPED_FILES */

int
stream_mapfile_create (stream_t *pstream)
{
#ifndef _POSIX_MAPPED_FILES
  return ENOSYS;
#else
  struct _ms *ms;

  if (pstream == NULL)
    return MU_ERROR_INVALID_PARAMETER;

  ms = calloc (1, sizeof *ms);
  if (ms == NULL)
    return MU_ERROR_NO_MEMORY;

  ms->base.vtable = &_map_vtable;
  ms->ref = 1;
  ms->fd = -1;
  ms->offset = -1;
  ms->flags = 0;
  ms->mflags = 0;
  monitor_create (&(ms->lock));
  *pstream = &ms->base;

  return 0;
#endif /* _POSIX_MAPPED_FILES */
}
