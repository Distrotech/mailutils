/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002, 2004, 
   2005, 2006, 2007, 2008, 2009, 2010 Free Software Foundation, Inc.

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

#include <mailutils/types.h>
#include <mailutils/alloc.h>
#include <mailutils/error.h>
#include <mailutils/errno.h>
#include <mailutils/nls.h>
#include <mailutils/stream.h>
#include <mailutils/sys/memory_stream.h>
#include <mailutils/mutil.h>

static void
_memory_done (mu_stream_t stream)
{
  struct _mu_memory_stream *mfs = (struct _mu_memory_stream *) stream;
  if (mfs && mfs->ptr != NULL)
    free (mfs->ptr);
}

static int
_memory_read (mu_stream_t stream, char *optr, size_t osize, size_t *nbytes)
{
  struct _mu_memory_stream *mfs = (struct _mu_memory_stream *) stream;
  size_t n = 0;
  if (mfs->ptr != NULL && ((size_t)mfs->offset <= mfs->size))
    {
      n = ((mfs->offset + osize) > mfs->size) ?
	    mfs->size - mfs->offset :  osize;
      memcpy (optr, mfs->ptr + mfs->offset, n);
      mfs->offset += n;
    }
  if (nbytes)
    *nbytes = n;
  return 0;
}

static int
_memory_write (mu_stream_t stream, const char *iptr, size_t isize,
	       size_t *nbytes)
{
  struct _mu_memory_stream *mfs = (struct _mu_memory_stream *) stream;
  
  if (mfs->capacity < mfs->offset + isize)
    {
      /* Realloc by fixed blocks of MU_STREAM_MEMORY_BLOCKSIZE. */
      size_t newsize = MU_STREAM_MEMORY_BLOCKSIZE *
	(((mfs->offset + isize) / MU_STREAM_MEMORY_BLOCKSIZE) + 1);
      char *tmp = mu_realloc (mfs->ptr, newsize);
      if (tmp == NULL)
	return ENOMEM;
      mfs->ptr = tmp;
      mfs->capacity = newsize;
    }

  memcpy (mfs->ptr + mfs->offset, iptr, isize);
  
  mfs->offset += isize;

  if (mfs->offset > mfs->size)
    mfs->size = mfs->offset;

  if (nbytes)
    *nbytes = isize;
  return 0;
}

static int
_memory_truncate (mu_stream_t stream, mu_off_t len)
{
  struct _mu_memory_stream *mfs = (struct _mu_memory_stream *) stream;

  if (len > (mu_off_t) mfs->size)
    {
      char *tmp = mu_realloc (mfs->ptr, len);
      if (tmp == NULL)
	return ENOMEM;
      mfs->ptr = tmp;
      mfs->capacity = len;
    }
  mfs->size = len;
  if (mfs->offset > mfs->size)
    mfs->offset = mfs->size;
  return 0;
}

static int
_memory_size (mu_stream_t stream, mu_off_t *psize)
{
  struct _mu_memory_stream *mfs = (struct _mu_memory_stream *) stream;
  if (psize)
    *psize = mfs->size;
  return 0;
}

static int
_memory_close (mu_stream_t stream)
{
  struct _mu_memory_stream *mfs = (struct _mu_memory_stream *) stream;
  if (mfs->ptr)
    free (mfs->ptr);
  mfs->ptr = NULL;
  mfs->size = 0;
  mfs->capacity = 0;
  return 0;
}

static int
_memory_open (mu_stream_t stream)
{
  struct _mu_memory_stream *mfs = (struct _mu_memory_stream *) stream;
  int status = 0;

  /* Close any previous stream. */
  if (mfs->ptr)
    free (mfs->ptr);
  mfs->ptr = NULL;
  mfs->size = 0;
  mfs->capacity = 0;

  return status;
}

static int
_memory_ioctl (struct _mu_stream *stream, int code, void *ptr)
{
  struct _mu_memory_stream *mfs = (struct _mu_memory_stream *) stream;
  mu_transport_t (*ptrans)[2];
  
  switch (code)
    {
    case MU_IOCTL_GET_TRANSPORT:
      if (!ptr)
	return EINVAL;
      ptrans = ptr;
      (*ptrans)[0] = (mu_transport_t) mfs->ptr;
      (*ptrans)[1] = NULL;
      break;

    default:
      return EINVAL;
    }
  return 0;
}

static int
_memory_seek (struct _mu_stream *stream, mu_off_t off, mu_off_t *presult)
{ 
  struct _mu_memory_stream *mfs = (struct _mu_memory_stream *) stream;

  if (off < 0)
    return ESPIPE;
  mfs->offset = off;
  *presult = off;
  return 0;
}

int
mu_memory_stream_create (mu_stream_t *pstream, int flags)
{
  struct _mu_memory_stream *str;

  if (!flags)
    flags = MU_STREAM_RDWR;
  str = (struct _mu_memory_stream *) _mu_stream_create (sizeof (*str),
							flags | MU_STREAM_SEEK);
  
  if (!str)
    return ENOMEM;

  str->ptr = NULL;
  str->size = 0;
  str->offset = 0;
  str->capacity = 0;

  str->stream.open = _memory_open;
  str->stream.close = _memory_close;
  str->stream.read = _memory_read;
  str->stream.write = _memory_write;
  str->stream.truncate = _memory_truncate;
  str->stream.size = _memory_size;
  str->stream.done = _memory_done;
  str->stream.ctl = _memory_ioctl;
  str->stream.seek = _memory_seek;

  *pstream = (mu_stream_t) str;

  return 0;
}
  
