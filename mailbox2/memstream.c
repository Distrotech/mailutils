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

#include <mailutils/error.h>
#include <mailutils/sys/memstream.h>

static int
_memory_add_ref (stream_t stream)
{
  int status;
  struct _memory_stream *mem = (struct _memory_stream *)stream;
  monitor_lock (mem->lock);
  status = ++mem->ref;
  monitor_unlock (mem->lock);
  return status;
}

static int
_memory_destroy (stream_t stream)
{
  struct _memory_stream *mem = (struct _memory_stream *)stream;
  if (mem && mem->ptr != NULL)
    free (mem->ptr);
  free (mem);
  return 0;
}

static int
_memory_release (stream_t stream)
{
  int status;
  struct _memory_stream *mem = (struct _memory_stream *)stream;
  monitor_lock (mem->lock);
  status = --mem->ref;
  if (status <= 0)
    {
      monitor_unlock (mem->lock);
      _memory_destroy (stream);
      return 0;
    }
  monitor_unlock (mem->lock);
  return status;
}

static int
_memory_read (stream_t stream, void *optr, size_t osize, size_t *nbytes)
{
  struct _memory_stream *mem = (struct _memory_stream *)stream;
  size_t n = 0;
  if (mem->ptr != NULL && (mem->offset < (off_t)mem->size))
    {
      n = ((mem->offset + osize) > mem->size) ?
	mem->size - mem->offset :  osize;
      memcpy (optr, mem->ptr + mem->offset, n);
      mem->offset += n;
    }
  if (nbytes)
    *nbytes = n;
  return 0;
}

static int
_memory_readline (stream_t stream, char *optr, size_t osize, size_t *nbytes)
{
  struct _memory_stream *mem = (struct _memory_stream *)stream;
  char *nl;
  size_t n = 0;
  if (mem->ptr && (mem->offset < (off_t)mem->size))
    {
      /* Save space for the null byte.  */
      osize--;
      nl = memchr (mem->ptr + mem->offset, '\n', mem->size - mem->offset);
      n = (nl) ? nl - (mem->ptr + mem->offset) + 1 : mem->size - mem->offset;
      n = (n > osize)  ? osize : n;
      memcpy (optr, mem->ptr + mem->offset, n);
      optr[n] = '\0';
      mem->offset += n;
    }
  if (nbytes)
    *nbytes = n;
  return 0;
}

static int
_memory_write (stream_t stream, const void *iptr, size_t isize, size_t *nbytes)
{
  struct _memory_stream *mem = (struct _memory_stream *)stream;

  /* Bigger we have to realloc.  */
  if (mem->size < (mem->offset + isize))
    {
      char *tmp =  realloc (mem->ptr, mem->offset + isize);
      if (tmp == NULL)
	return ENOMEM;
      mem->ptr = tmp;
      mem->size = mem->offset + isize;
    }

  memcpy (mem->ptr + mem->offset, iptr, isize);
  mem->offset += isize;
  if (nbytes)
    *nbytes = isize;
  return 0;
}

static int
_memory_truncate (stream_t stream, off_t len)
{
  struct _memory_stream *mem = (struct _memory_stream *)stream;

  if (len == 0)
    {
      free (mem->ptr);
      mem->ptr = NULL;
    }
  else
    {
      char *tmp = realloc (mem, len);
      if (tmp == NULL)
	return ENOMEM;
      mem->ptr = tmp;
    }
  mem->size = len;
  mem->offset = len;
  return 0;
}

static int
_memory_get_size (stream_t stream, off_t *psize)
{
  struct _memory_stream *mem = (struct _memory_stream *)stream;
  if (psize)
    *psize = mem->size;
  return 0;
}

static int
_memory_close (stream_t stream)
{
  struct _memory_stream *mem = (struct _memory_stream *)stream;
  if (mem->ptr)
    free (mem->ptr);
  mem->ptr = NULL;
  mem->size = 0;
  mem->offset = 0;
  return 0;
}

static int
_memory_flush (stream_t stream)
{
  (void)stream;
  return 0;
}

static int
_memory_get_fd (stream_t stream, int *pfd)
{
  (void)stream; (void)pfd;
  return MU_ERROR_NOT_SUPPORTED;
}

static int
_memory_get_flags (stream_t stream, int *flags)
{
  struct _memory_stream *mem = (struct _memory_stream *)stream;
  if (flags == NULL)
    return MU_ERROR_INVALID_PARAMETER;
  *flags = mem->flags;
  return 0;
}

static int
_memory_get_state (stream_t stream, enum stream_state *state)
{
  (void)stream;
  if (state == NULL)
    return MU_ERROR_INVALID_PARAMETER;
  *state = MU_STREAM_NO_STATE;
  return 0;
}

static int
_memory_seek (stream_t stream, off_t off, enum stream_whence whence)
{
  struct _memory_stream *mem = (struct _memory_stream *)stream;
  off_t noff = mem->offset;
  int err = 0;
  if (whence == MU_STREAM_WHENCE_SET)
      noff = off;
  else if (whence == MU_STREAM_WHENCE_CUR)
    noff += off;
  else if (whence == MU_STREAM_WHENCE_END)
    noff = mem->size + off;
  else
    noff = -1; /* error.  */
  if (noff >= 0)
    {
      if (noff > mem->offset)
        _memory_truncate (stream, noff);
      mem->offset = noff;
    }
  else
    err = MU_ERROR_INVALID_PARAMETER;
  return err;
}

static int
_memory_tell (stream_t stream, off_t *off)
{
  struct _memory_stream *mem = (struct _memory_stream *)stream;
  if (off == NULL)
    return MU_ERROR_INVALID_PARAMETER;
  *off = mem->offset;
  return 0;
}

static int
_memory_is_open (stream_t stream)
{
  (void)stream;
  return 1;
}

static int
_memory_is_readready (stream_t stream, int timeout)
{
  (void)stream;
  (void)timeout;
  return 1;
}

static int
_memory_is_writeready (stream_t stream, int timeout)
{
  (void)stream;
  (void)timeout;
  return 1;
}

static int
_memory_is_exceptionpending (stream_t stream, int timeout)
{
  (void)stream;
  (void)timeout;
  return 0;
}


static int
_memory_open (stream_t stream, const char *filename, int port, int flags)
{
  struct _memory_stream *mem = (struct _memory_stream *)stream;

  (void)port; /* Ignored.  */
  (void)filename; /* Ignored.  */
  (void)flags; /* Ignored.  */

  /* Close any previous file.  */
  if (mem->ptr)
    free (mem->ptr);
  mem->ptr = NULL;
  mem->size = 0;
  mem->offset = 0;
  mem->flags = flags;
  return 0;
}

static struct _stream_vtable _mem_vtable =
{
  _memory_add_ref,
  _memory_release,
  _memory_destroy,

  _memory_open,
  _memory_close,

  _memory_read,
  _memory_readline,
  _memory_write,

  _memory_seek,
  _memory_tell,

  _memory_get_size,
  _memory_truncate,
  _memory_flush,

  _memory_get_fd,
  _memory_get_flags,
  _memory_get_state,

  _memory_is_readready,
  _memory_is_writeready,
  _memory_is_exceptionpending,

  _memory_is_open
};

int
stream_memory_create (stream_t *pstream)
{
  struct _memory_stream *mem;

  if (pstream == NULL)
    return MU_ERROR_INVALID_PARAMETER;

  mem = calloc (1, sizeof (*mem));
  if (mem == NULL)
    return MU_ERROR_NO_MEMORY;

  mem->base.vtable = &_mem_vtable;
  mem->ref = 1;
  mem->ptr = NULL;
  mem->size = 0;
  mem->offset = 0;
  mem->flags = 0;
  monitor_create (&(mem->lock));
  *pstream = &mem->base;

  return 0;
}
