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


#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <io0.h>

#ifdef _POSIX_MAPPED_FILES

#include <sys/mman.h>

struct _mapfile_stream
{
  int fd;
  int flags;
  char *ptr;
  size_t size;
};

static void
_mapfile_destroy (stream_t stream)
{
  struct _mapfile_stream *mfs = stream->owner;

  if (mfs->ptr)
    {
      munmap (mfs->ptr, mfs->size);
      close (mfs->fd);
    }
  free (mfs);
}

static int
_mapfile_read (stream_t stream, char *optr, size_t osize,
	    off_t offset, size_t *nbytes)
{
  struct _mapfile_stream *mfs = stream->owner;
  size_t n;

  if (offset >= (off_t)mfs->size)
    {
      if (nbytes)
	*nbytes = 0;
      return 0;
    }

  n = ((offset + osize) > mfs->size) ? mfs->size - offset :  osize;
  memcpy (optr, mfs->ptr + offset, n);

  if (nbytes)
    *nbytes = n;
  return 0;
}

static int
_mapfile_readline (stream_t stream, char *optr, size_t osize,
		off_t offset, size_t *nbytes)
{
  struct _mapfile_stream *mfs = stream->owner;
  char *nl;
  size_t n = 0;

  /* save space for the null byte */
  osize--;
  if (offset >= (off_t)mfs->size)
    {
      if (nbytes)
	*nbytes = 0;
      return 0;
    }

  nl = memchr (mfs->ptr + offset, '\n', mfs->size - offset);
  n = nl - (mfs->ptr + offset) + 1;
  n = (n) > osize  ? osize : n;
  memcpy (optr, mfs->ptr + offset, n);
  if (nbytes)
    *nbytes = n;
  optr[n] = '\0';
  return 0;
}

static int
_mapfile_write (stream_t stream, const char *iptr, size_t isize,
	    off_t offset, size_t *nbytes)
{
  struct _mapfile_stream *mfs = stream->owner;

  if (! (mfs->flags & PROT_WRITE))
    return EACCES;

  /* not recommanded, really */
  /* bigger we have to remmap */
  if (mfs->size < (offset + isize))
    {
      if (munmap (mfs->ptr, mfs->size) != 0)
	{
	  int err = errno;
	  mfs->ptr = MAP_FAILED;
	  close (mfs->fd);
	  return err;
	}
      if (ftruncate (mfs->fd, offset + isize) != 0)
	return errno;
      mfs->ptr = mmap (0, offset + isize, mfs->flags, MAP_SHARED, mfs->fd, 0);
      if (mfs->ptr == MAP_FAILED)
	{
	  int err = errno;
	  close (mfs->fd);
	  return err;
	}
      mfs->size = offset + isize;
    }

  memcpy (mfs->ptr + offset, iptr, isize);
  if (nbytes)
    *nbytes = isize;
  return 0;
}

static int
_mapfile_truncate (stream_t stream, off_t len)
{
  struct _mapfile_stream *mfs = stream->owner;
  /* remap */
  if (munmap (mfs->ptr, mfs->size) != 0)
    {
      int err = errno;
      mfs->ptr = MAP_FAILED;
      close (mfs->fd);
      return err;
    }
  if (ftruncate (mfs->fd, len) != 0)
    return errno;
  mfs->ptr = mmap (0, len, mfs->flags, MAP_SHARED, mfs->fd, 0);
  if (mfs->ptr == MAP_FAILED)
    {
      int err = errno;
      close (mfs->fd);
      return err;
    }
  mfs->size = len;
  return 0;
}

static int
_mapfile_size (stream_t stream, off_t *psize)
{
  struct _mapfile_stream *mfs = stream->owner;
  struct stat stbuf;
  msync (mfs->ptr, mfs->size, MS_SYNC);
  if (fstat(mfs->fd, &stbuf) != 0)
    return errno;
  if (psize)
    *psize = stbuf.st_size;
  return 0;
}

static int
_mapfile_flush (stream_t stream)
{
  struct _mapfile_stream *mfs = stream->owner;
  return msync (mfs->ptr, mfs->size, MS_SYNC);
}

static int
_mapfile_get_fd (stream_t stream, int *pfd)
{
  struct _mapfile_stream *mfs = stream->owner;
  if (pfd)
    *pfd = mfs->fd;
  return 0;
}

static int
_mapfile_close (stream_t stream)
{
  struct _mapfile_stream *mfs = stream->owner;
  if (mfs->ptr)
    {
      if (munmap (mfs->ptr, mfs->size) != 0)
	return errno;
      mfs->ptr = NULL;
      if (close (mfs->fd) != 0)
	return errno;
      mfs->fd = -1;
    }
  return 0;
}

static int
_mapfile_open (stream_t stream, const char *filename, int port, int flags)
{
  struct _mapfile_stream *mfs = stream->owner;
  int mflag, flg;
  struct stat st;

  (void)port; /* shutup gcc */

  /* map the flags to the system equivalent */
  if (flags & MU_STREAM_WRITE)
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
    return ENOTSUP;
  else /* default */
    {
      mflag = PROT_READ;
      flg = O_RDONLY;
    }

  mfs->fd = open (filename, flg);
  if (mfs->fd < 0)
    return errno;
  if (fstat (mfs->fd, &st) != 0)
    {
      int err = errno;
      close (mfs->fd);
      return err;
    }
  mfs->size = st.st_size;
  mfs->ptr = mmap (0, mfs->size, mflag , MAP_SHARED, mfs->fd, 0);
  if (mfs->ptr == MAP_FAILED)
    {
      int err = errno;
      close (mfs->fd);
      return err;
    }
  mfs->flags = mflag;
  stream_set_flags (stream, flags |MU_STREAM_NO_CHECK, mfs);
  return 0;
}

#endif /* _POSIX_MAPPED_FILES */

int
mapfile_stream_create (stream_t *stream)
{
#ifndef _POSIX_MAPPED_FILES
  return ENOTSUP;
#else
  struct _mapfile_stream *fs;
  int ret;

  if (stream == NULL)
    return EINVAL;

  fs = calloc (1, sizeof (struct _mapfile_stream));
  if (fs == NULL)
    return ENOMEM;

  ret = stream_create (stream, MU_STREAM_NO_CHECK, fs);
  if (ret != 0)
    {
      free (fs);
      return ret;
    }

  stream_set_open (*stream, _mapfile_open, fs);
  stream_set_close (*stream, _mapfile_close, fs);
  stream_set_fd (*stream, _mapfile_get_fd, fs);
  stream_set_read (*stream, _mapfile_read, fs);
  stream_set_readline (*stream, _mapfile_readline, fs);
  stream_set_write (*stream, _mapfile_write, fs);
  stream_set_truncate (*stream, _mapfile_truncate, fs);
  stream_set_size (*stream, _mapfile_size, fs);
  stream_set_flush (*stream, _mapfile_flush, fs);
  stream_set_destroy (*stream, _mapfile_destroy, fs);
  return 0;
#endif /* _POSIX_MAPPED_FILES */
}
