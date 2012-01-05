/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2001, 2004, 2007, 2009-2012 Free Software
   Foundation, Inc.

   This library is free software; you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>. */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <mailutils/types.h>
#include <mailutils/alloc.h>
#include <mailutils/error.h>
#include <mailutils/errno.h>
#include <mailutils/nls.h>
#include <mailutils/stream.h>
#include <mailutils/sys/stream.h>
#include <mailutils/sys/mapfile_stream.h>

#ifdef _POSIX_MAPPED_FILES
#include <sys/mman.h>

#ifndef MAP_FAILED
# define MAP_FAILED (void*)-1
#endif

static void
_mapfile_done (mu_stream_t stream)
{
  struct _mu_mapfile_stream *mfs = (struct _mu_mapfile_stream *)stream;

  if (mfs->ptr != MAP_FAILED)
    {
      if (mfs->ptr)
	munmap (mfs->ptr, mfs->size);
      close (mfs->fd);
    }
  free (mfs->filename);
}

static int
_mapfile_read (mu_stream_t stream, char *optr, size_t osize,
	       size_t *nbytes)
{
  struct _mu_mapfile_stream *mfs = (struct _mu_mapfile_stream *) stream;
  size_t n = 0;

  if (mfs->ptr == MAP_FAILED)
    return EINVAL;

  if (mfs->offset < (mu_off_t) mfs->size)
    {
      n = ((mfs->offset + osize) > mfs->size) ?
	    mfs->size - mfs->offset : osize;
      memcpy (optr, mfs->ptr + mfs->offset, n);
      mfs->offset += n;
    }

  if (nbytes)
    *nbytes = n;
  return 0;
}

static int
_mapfile_write (mu_stream_t stream, const char *iptr, size_t isize,
		size_t *nbytes)
{
  struct _mu_mapfile_stream *mfs = (struct _mu_mapfile_stream *) stream;

  if (mfs->ptr == MAP_FAILED)
    return EINVAL;

  if (!(mfs->flags & PROT_WRITE))
    return EACCES;

  if (mfs->size < (mfs->offset + isize))
    {
      if (mfs->ptr && munmap (mfs->ptr, mfs->size) != 0)
	{
	  int err = errno;
	  mfs->ptr = MAP_FAILED;
	  close (mfs->fd);
	  return err;
	}
      if (ftruncate (mfs->fd, mfs->offset + isize) != 0)
	return errno;
      mfs->ptr = mmap (0, mfs->offset + isize, mfs->flags,
		       MAP_SHARED, mfs->fd, 0);
      if (mfs->ptr == MAP_FAILED)
	{
	  int err = errno;
	  close (mfs->fd);
	  return err;
	}
      mfs->size = mfs->offset + isize;
    }

  if (isize)
    {
      memcpy (mfs->ptr + mfs->offset, iptr, isize);
      mfs->offset += isize;
    }
  
  if (nbytes)
    *nbytes = isize;
  return 0;
}

static int
_mapfile_truncate (mu_stream_t stream, mu_off_t len)
{
  struct _mu_mapfile_stream *mfs = (struct _mu_mapfile_stream *) stream;
  
  if (mfs->ptr == MAP_FAILED)
    return EINVAL;
  /* Remap.  */
  if (mfs->ptr && munmap (mfs->ptr, mfs->size) != 0)
    {
      int err = errno;
      mfs->ptr = MAP_FAILED;
      close (mfs->fd);
      return err;
    }
  if (ftruncate (mfs->fd, len) != 0)
    return errno;
   mfs->ptr = len ? mmap (0, len, mfs->flags, MAP_SHARED, mfs->fd, 0) : NULL;
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
_mapfile_size (mu_stream_t stream, mu_off_t *psize)
{
  struct _mu_mapfile_stream *mfs = (struct _mu_mapfile_stream *) stream;
  struct stat stbuf;
  int err = 0;

  if (mfs->ptr == MAP_FAILED)
    return EINVAL;
  if (mfs->ptr && (mfs->flags & PROT_WRITE))
    msync (mfs->ptr, mfs->size, MS_SYNC);
  if (fstat (mfs->fd, &stbuf) != 0)
    return errno;
  if (mfs->size != (size_t) stbuf.st_size)
    {
      if (mfs->ptr)
	err = munmap (mfs->ptr, mfs->size);
      if (err == 0)
	{
	  mfs->size = stbuf.st_size;
	  if (mfs->size)
	    {
	      mfs->ptr = mmap (0, mfs->size, mfs->flags , MAP_SHARED,
			       mfs->fd, 0);
	      if (mfs->ptr == MAP_FAILED)
		err = errno;
	    }
	  else
	    mfs->ptr = NULL;
	}
      else
	err = errno;
    }
  if (err != 0)
    {
      mfs->ptr = MAP_FAILED;
      close (mfs->fd);
      mfs->fd = -1;
    }
  else
    {
      if (psize)
	*psize = stbuf.st_size;
    }
  return err;
}

static int
_mapfile_flush (mu_stream_t stream)
{
  struct _mu_mapfile_stream *mfs = (struct _mu_mapfile_stream *) stream;
  if (mfs->ptr != MAP_FAILED && mfs->ptr != NULL && (mfs->flags & PROT_WRITE))
    return msync (mfs->ptr, mfs->size, MS_SYNC);
  return 0;
}

static int
_mapfile_ioctl (struct _mu_stream *str, int code, int opcode, void *ptr)
{
  struct _mu_mapfile_stream *mfs = (struct _mu_mapfile_stream *) str;
  
  switch (code)
    {
    case MU_IOCTL_TRANSPORT:
      if (!ptr)
	return EINVAL;
      else
	{
	  mu_transport_t *ptrans = ptr;
	  switch (opcode)
	    {
	    case MU_IOCTL_OP_GET:
	      ptrans[0] = (mu_transport_t) mfs->fd;
	      ptrans[1] = NULL;
	      break;
	    case MU_IOCTL_OP_SET:
	      return ENOSYS;
	    default:
	      return EINVAL;
	    }
	}
      break;

    case MU_IOCTL_TRANSPORT_BUFFER:
      if (!ptr)
	return EINVAL;
      else
	{
	  struct mu_buffer_query *qp = ptr;
	  switch (opcode)
	    {
	    case MU_IOCTL_OP_GET:
	      return mu_stream_get_buffer (str, qp);
	    case MU_IOCTL_OP_SET:
	      return mu_stream_set_buffer (str, qp->buftype, qp->bufsize);
	    default:
	      return EINVAL;
	    }
	}
      break;
      
    default:
      return ENOSYS;
    }
  return 0;
}

static int
_mapfile_close (mu_stream_t stream)
{
  struct _mu_mapfile_stream *mfs = (struct _mu_mapfile_stream *) stream;
  int err = 0;

  if (mfs->ptr != MAP_FAILED)
    {
      if (mfs->ptr && munmap (mfs->ptr, mfs->size) != 0)
	err = errno;
      if (close (mfs->fd) != 0)
	err = errno;
      mfs->ptr = MAP_FAILED;
      mfs->fd = -1;
    }
  return err;
}

static int
_mapfile_open (mu_stream_t stream)
{
  struct _mu_mapfile_stream *mfs = (struct _mu_mapfile_stream *) stream;
  int mflag, flg;
  struct stat st;
  char *filename = mfs->filename;
  int flags;

  mu_stream_get_flags (stream, &flags);

  /* Close any previous file.  */
  if (mfs->ptr != MAP_FAILED)
    {
      if (mfs->ptr)
	munmap (mfs->ptr, mfs->size);
      mfs->ptr = MAP_FAILED;
    }
  if (mfs->fd != -1)
    {
      close (mfs->fd);
      mfs->fd = -1;
    }
  /* Map the flags to the system equivalent */
  if ((flags & MU_STREAM_RDWR) == MU_STREAM_RDWR)
    {
      mflag = PROT_READ | PROT_WRITE;
      flg = O_RDWR;
    }
  else if (flags & MU_STREAM_WRITE)
    {
      mflag = PROT_WRITE;
      flg = O_WRONLY;
    }
  else if (flags & MU_STREAM_CREAT)
    return ENOSYS;
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
  if (mfs->size)
    {
      mfs->ptr = mmap (0, mfs->size, mflag , MAP_SHARED, mfs->fd, 0);
      if (mfs->ptr == MAP_FAILED)
	{
	  int err = errno;
	  close (mfs->fd);
	  mfs->ptr = MAP_FAILED;
	  return err;
	}
    }
  else
    mfs->ptr = NULL;
  mfs->flags = mflag;
  return 0;
}

static int
_mapfile_seek (struct _mu_stream *str, mu_off_t off, mu_off_t *presult)
{ 
  struct _mu_mapfile_stream *mfs = (struct _mu_mapfile_stream *) str;
  
  if (off < 0 || off > mfs->size)
    return ESPIPE;
  mfs->offset = off;
  *presult = off;
  return 0;
}

#endif /* _POSIX_MAPPED_FILES */

int
mu_mapfile_stream_create (mu_stream_t *pstream, const char *filename,
			  int flags)
{
#ifndef _POSIX_MAPPED_FILES
  return ENOSYS;
#else
  int rc;
  mu_stream_t stream;
  struct _mu_mapfile_stream *str =
    (struct _mu_mapfile_stream *) _mu_stream_create (sizeof (*str),
						     flags | MU_STREAM_SEEK);
  if (!str)
    return ENOMEM;

  str->filename = mu_strdup (filename);
  if (!str->filename)
    {
      free (str);
      return ENOMEM;
    }
  str->fd = -1;
  str->ptr = MAP_FAILED;

  str->stream.open = _mapfile_open;
  str->stream.close = _mapfile_close;
  str->stream.ctl = _mapfile_ioctl;
  str->stream.read = _mapfile_read;
  str->stream.write = _mapfile_write;
  str->stream.truncate = _mapfile_truncate;
  str->stream.size = _mapfile_size;
  str->stream.flush = _mapfile_flush;
  str->stream.done = _mapfile_done;
  str->stream.seek = _mapfile_seek;

  stream = (mu_stream_t) str;
  rc = mu_stream_open (stream);
  if (rc)
    mu_stream_destroy (&stream);
  else
    *pstream = stream;
  return rc;
#endif
}


