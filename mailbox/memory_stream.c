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
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>

#include <mailutils/stream.h>

struct _memory_stream
{
  char *ptr;
  size_t size;
};

static void
_memory_destroy (stream_t stream)
{
  struct _memory_stream *mfs = stream_get_owner (stream);
  if (mfs && mfs->ptr != NULL)
    free (mfs->ptr);
  free (mfs);
}

static int
_memory_read (stream_t stream, char *optr, size_t osize,
	      off_t offset, size_t *nbytes)
{
  struct _memory_stream *mfs = stream_get_owner (stream);
  size_t n = 0;
  if (mfs->ptr != NULL && (offset <= (off_t)mfs->size))
    {
      n = ((offset + osize) > mfs->size) ? mfs->size - offset :  osize;
      memcpy (optr, mfs->ptr + offset, n);
    }
  if (nbytes)
    *nbytes = n;
  return 0;
}

static int
_memory_readline (stream_t stream, char *optr, size_t osize,
		  off_t offset, size_t *nbytes)
{
  struct _memory_stream *mfs = stream_get_owner (stream);
  char *nl;
  size_t n = 0;
  if (mfs->ptr && (offset < (off_t)mfs->size))
    {
      /* Save space for the null byte.  */
      osize--;
      nl = memchr (mfs->ptr + offset, '\n', mfs->size - offset);
      n = (nl) ? (size_t)(nl - (mfs->ptr + offset) + 1) : mfs->size - offset;
      n = (n > osize)  ? osize : n;
      memcpy (optr, mfs->ptr + offset, n);
      optr[n] = '\0';
    }
  if (nbytes)
    *nbytes = n;
  return 0;
}

static int
_memory_write (stream_t stream, const char *iptr, size_t isize,
	       off_t offset, size_t *nbytes)
{
  struct _memory_stream *mfs = stream_get_owner (stream);

  /* Bigger we have to realloc.  */
  if (mfs->size < (offset + isize))
    {
      char *tmp =  realloc (mfs->ptr, offset + isize);
      if (tmp == NULL)
	return ENOMEM;
      mfs->ptr = tmp;
      mfs->size = offset + isize;
    }

  memcpy (mfs->ptr + offset, iptr, isize);
  if (nbytes)
    *nbytes = isize;
  return 0;
}

static int
_memory_truncate (stream_t stream, off_t len)
{
  struct _memory_stream *mfs = stream_get_owner (stream);

  if (len == 0)
    {
      free (mfs->ptr);
      mfs->ptr = NULL;
    }
  else
    {
      char *tmp = realloc (mfs, len);
      if (tmp == NULL)
	return ENOMEM;
      mfs->ptr = tmp;
    }
  mfs->size = len;
  return 0;
}

static int
_memory_size (stream_t stream, off_t *psize)
{
  struct _memory_stream *mfs = stream_get_owner (stream);
  if (psize)
    *psize = mfs->size;
  return 0;
}

static int
_memory_close (stream_t stream)
{
  struct _memory_stream *mfs = stream_get_owner (stream);
  if (mfs->ptr)
    {
      free (mfs->ptr);
      mfs->ptr = NULL;
      mfs->size = 0;
    }
  return 0;
}

static int
_memory_open (stream_t stream, const char *filename, int port, int flags)
{
  struct _memory_stream *mfs = stream_get_owner (stream);

  (void)port; /* Ignored.  */
  (void)filename; /* Ignored.  */
  (void)flags; /* Ignored.  */

  /* Close any previous file.  */
  if (mfs->ptr)
    {
      free (mfs->ptr);
      mfs->ptr = NULL;
      mfs->size = 0;
    }
  stream_set_flags (stream, flags |MU_STREAM_NO_CHECK);
  return 0;
}

int
memory_stream_create (stream_t *stream)
{
  struct _memory_stream *mfs;
  int ret;

  if (stream == NULL)
    return EINVAL;

  mfs = calloc (1, sizeof (*mfs));
  if (mfs == NULL)
    return ENOMEM;

  mfs->ptr = NULL;
  mfs->size = 0;

  ret = stream_create (stream, MU_STREAM_NO_CHECK, mfs);
  if (ret != 0)
    {
      free (mfs);
      return ret;
    }

  stream_set_open (*stream, _memory_open, mfs);
  stream_set_close (*stream, _memory_close, mfs);
  stream_set_read (*stream, _memory_read, mfs);
  stream_set_readline (*stream, _memory_readline, mfs);
  stream_set_write (*stream, _memory_write, mfs);
  stream_set_truncate (*stream, _memory_truncate, mfs);
  stream_set_size (*stream, _memory_size, mfs);
  stream_set_destroy (*stream, _memory_destroy, mfs);
  return 0;
}
