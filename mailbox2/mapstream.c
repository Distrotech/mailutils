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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <mailutils/sys/mapstream.h>
#include <mailutils/error.h>

#ifdef _POSIX_MAPPED_FILES
#include <sys/mman.h>

#ifndef MAP_FAILED
# define MAP_FAILED (void*)-1
#endif

int
_stream_mmap_ref (stream_t stream)
{
  struct _stream_mmap *ms = (struct _stream_mmap *)stream;
  return mu_refcount_inc (ms->refcount);
}

void
_stream_mmap_destroy (stream_t *pstream)
{
  struct _stream_mmap *ms = (struct _stream_mmap *)*pstream;

  if (mu_refcount_dec (ms->refcount) == 0)
    {
      mu_refcount_destroy (&ms->refcount);
      free (ms);
    }
}

int
_stream_mmap_read (stream_t stream, void *optr, size_t osize,
		   off_t offset, size_t *nbytes)
{
  struct _stream_mmap *ms = (struct _stream_mmap *)stream;
  size_t n = 0;

  mu_refcount_lock (ms->refcount);
  if (ms->ptr != MAP_FAILED && ms->ptr)
    {
      if (offset < (off_t)ms->size)
	{
	  n = ((offset + osize) > ms->size) ? ms->size - offset :  osize;
	  memcpy (optr, ms->ptr + offset, n);
	}
    }
  mu_refcount_unlock (ms->refcount);

  if (nbytes)
    *nbytes = n;
  return 0;
}

int
_stream_mmap_readline (stream_t stream, char *optr, size_t osize,
		       off_t offset, size_t *nbytes)
{
  struct _stream_mmap *ms = (struct _stream_mmap *)stream;
  size_t n = 0;

  mu_refcount_lock (ms->refcount);
  if (ms->ptr != MAP_FAILED && ms->ptr)
    {
      if (offset < (off_t)ms->size)
	{
	  /* Save space for the null byte.  */
	  char *nl;
	  osize--;
	  nl = memchr (ms->ptr + offset, '\n', ms->size - offset);
	  n = (nl) ? (size_t)(nl - (ms->ptr + offset) + 1) : ms->size - offset;
	  n = (n > osize)  ? osize : n;
	  memcpy (optr, ms->ptr + offset, n);
	  optr[n] = '\0';
	  offset += n;
	}
    }
  mu_refcount_unlock (ms->refcount);

  if (nbytes)
    *nbytes = n;
  return 0;
}

int
_stream_mmap_write (stream_t stream, const void *iptr, size_t isize,
		    off_t offset, size_t *nbytes)
{
  struct _stream_mmap *ms = (struct _stream_mmap *)stream;
  int err = 0;
  size_t n = 0;

  mu_refcount_lock (ms->refcount);
  if (ms->mflags & PROT_WRITE)
    {
      /* Bigger we have to remmap.  */
      if (ms->size < (offset + isize))
	{
	  if (ms->ptr != MAP_FAILED && munmap (ms->ptr, ms->size) == 0)
	    {
	      ms->ptr = MAP_FAILED;
	      if (ftruncate (ms->fd, offset + isize) == 0)
		{
		  ms->ptr = mmap (0, offset + isize, ms->mflags,
				  MAP_SHARED, ms->fd, 0);
		  if (ms->ptr != MAP_FAILED)
		    ms->size = offset + isize;
		}
	    }
	}

      if (ms->ptr != MAP_FAILED)
	{
	  if (isize > 0)
	    memcpy (ms->ptr + offset, iptr, isize);
	  n = isize;
	}
      else
	err = MU_ERROR_IO;
    }
  else
    err = MU_ERROR_IO;

  mu_refcount_lock (ms->refcount);

  if (nbytes)
    *nbytes = n;
  return err;
}

int
_stream_mmap_truncate (stream_t stream, off_t len)
{
  struct _stream_mmap *ms = (struct _stream_mmap *)stream;
  int err = 0;

  mu_refcount_lock (ms->refcount);
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
  mu_refcount_unlock (ms->refcount);
  return err;
}

int
_stream_mmap_get_size (stream_t stream, off_t *psize)
{
  struct _stream_mmap *ms = (struct _stream_mmap *)stream;
  struct stat stbuf;
  int err = 0;

  mu_refcount_lock (ms->refcount);
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
  mu_refcount_unlock (ms->refcount);
  if (psize)
    *psize = stbuf.st_size;
  return err;
}

int
_stream_mmap_flush (stream_t stream)
{
  int err = 0;
  struct _stream_mmap *ms = (struct _stream_mmap *)stream;
  mu_refcount_lock (ms->refcount);
  if (ms->ptr != MAP_FAILED && ms->ptr != NULL)
    err = msync (ms->ptr, ms->size, MS_SYNC);
  mu_refcount_unlock (ms->refcount);
  return 0;
}

int
_stream_mmap_get_fd (stream_t stream, int *pfd)
{
  struct _stream_mmap *ms = (struct _stream_mmap *)stream;
  if (pfd)
    *pfd = ms->fd;
  return 0;
}

int
_stream_mmap_get_flags (stream_t stream, int *flags)
{
  struct _stream_mmap *ms = (struct _stream_mmap *)stream;
  if (flags == NULL)
    return MU_ERROR_INVALID_PARAMETER;
  *flags = ms->flags;
  return 0;
}

int
_stream_mmap_get_state (stream_t stream, enum stream_state *state)
{
  (void)stream;
  if (state == NULL)
    return MU_ERROR_INVALID_PARAMETER;
  *state = MU_STREAM_NO_STATE;
  return 0;
}

int
_stream_mmap_is_seekable (stream_t stream)
{
  off_t off;
  return _stream_mmap_tell (stream, &off) == 0;
}

int
_stream_mmap_tell (stream_t stream, off_t *off)
{
  struct _stream_mmap *ms = (struct _stream_mmap *)stream;
  int status = 0;
  if (off == NULL)
    return MU_ERROR_INVALID_PARAMETER;
  *off = lseek (ms->fd, 0, SEEK_SET);
  if (*off == -1)
    {
      status = errno;
      *off = 0;
    }
  return status;
}

int
_stream_mmap_close (stream_t stream)
{
  struct _stream_mmap *ms = (struct _stream_mmap *)stream;
  int err = 0;
  mu_refcount_lock (ms->refcount);
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
  mu_refcount_unlock (ms->refcount);
  return err;
}

int
_stream_mmap_is_open (stream_t stream)
{
  struct _stream_mmap *ms = (struct _stream_mmap *)stream;
  return (ms->ptr == MAP_FAILED) ? 0 : 1;
}

int
_stream_mmap_is_readready (stream_t stream, int timeout)
{
  (void)timeout;
  return _stream_mmap_is_open (stream);
}

int
_stream_mmap_is_writeready (stream_t stream, int timeout)
{
  (void)timeout;
  return _stream_mmap_is_open (stream);
}

int
_stream_mmap_is_exceptionpending (stream_t stream, int timeout)
{
  (void)stream;
  (void)timeout;
  return 0;
}

int
_stream_mmap_open (stream_t stream, const char *filename, int port, int flags)
{
  struct _stream_mmap *ms = (struct _stream_mmap *)stream;
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

  /* Anonymous Mapping.  */
  if (filename == NULL)
    {
#ifdef MAP_ANON
      mflag |= MAP_ANON;
#else
      filename = "/dev/zero";
      ms->fd = open (filename, flg);
#endif
    }
  else
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

static struct _stream_vtable _stream_mmap_vtable =
{
  _stream_mmap_ref,
  _stream_mmap_destroy,

  _stream_mmap_open,
  _stream_mmap_close,

  _stream_mmap_read,
  _stream_mmap_readline,
  _stream_mmap_write,

  _stream_mmap_tell,

  _stream_mmap_get_size,
  _stream_mmap_truncate,
  _stream_mmap_flush,

  _stream_mmap_get_fd,
  _stream_mmap_get_flags,
  _stream_mmap_get_state,

  _stream_mmap_is_seekable,
  _stream_mmap_is_readready,
  _stream_mmap_is_writeready,
  _stream_mmap_is_exceptionpending,

  _stream_mmap_is_open
};

int
_stream_mmap_ctor (struct _stream_mmap *ms)
{
  mu_refcount_create (&ms->refcount);
  if (ms->refcount == NULL)
    return MU_ERROR_NO_MEMORY;
  ms->fd = -1;
  ms->flags = 0;
  ms->mflags = 0;
  ms->base.vtable = &_stream_mmap_vtable;
  return 0;
}

void
_stream_mmap_dtor (stream_t stream)
{
  struct _stream_mmap *ms = (struct _stream_mmap *)stream;
  if (ms)
    {
      mu_refcount_destroy (&ms->refcount);
      ms->fd = -1;
      ms->mflags = 0;
      ms->ptr = MAP_FAILED;
    }
}

#endif /* _POSIX_MAPPED_FILES */

int
stream_mmap_create (stream_t *pstream)
{
#ifndef _POSIX_MAPPED_FILES
  return ENOSYS;
#else
  struct _stream_mmap *ms;
  int status;

  if (pstream == NULL)
    return MU_ERROR_INVALID_PARAMETER;

  ms = calloc (1, sizeof *ms);
  if (ms == NULL)
    return MU_ERROR_NO_MEMORY;

  status = _stream_mmap_ctor (ms);
  if (status != 0)
    return status;
  *pstream = &ms->base;
  return 0;
#endif /* _POSIX_MAPPED_FILES */
}
