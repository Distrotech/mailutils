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
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <mailutils/error.h>
#include <mailutils/sys/memstream.h>

#undef min
#define min(a,b) ((a) < (b) ? (a) : (b))

int
_stream_memory_ref (stream_t stream)
{
  struct _stream_memory *mem = (struct _stream_memory *)stream;
  return mu_refcount_inc (mem->refcount);
}

void
_stream_memory_destroy (stream_t *pstream)
{
  struct _stream_memory *mem = (struct _stream_memory *)*pstream;
  if (mu_refcount_dec (mem->refcount) == 0)
    {
      mu_refcount_destroy (&mem->refcount);
      free (mem);
    }
}

int
_stream_memory_read (stream_t stream, void *optr, size_t osize,
		     off_t offset, size_t *nbytes)
{
  struct _stream_memory *mem = (struct _stream_memory *)stream;
  size_t n = 0;

  mu_refcount_lock (mem->refcount);
  if (mem->ptr != NULL && (offset < (off_t)mem->size))
    {
      n = ((offset + osize) > mem->size) ? mem->size - offset :  osize;
      memcpy (optr, mem->ptr + offset, n);
    }
  mu_refcount_unlock (mem->refcount);
  if (nbytes)
    *nbytes = n;
  return 0;
}

int
_stream_memory_readline (stream_t stream, char *optr, size_t osize,
			 off_t offset, size_t *nbytes)
{
  struct _stream_memory *mem = (struct _stream_memory *)stream;
  char *nl;
  size_t n = 0;
  mu_refcount_lock (mem->refcount);
  if (mem->ptr && (offset < (off_t)mem->size))
    {
      /* Save space for the null byte.  */
      osize--;
      nl = memchr (mem->ptr + offset, '\n', mem->size - offset);
      n = (nl) ? (size_t)(nl - (mem->ptr + offset) + 1) : mem->size - offset;
      n = min (n, osize);
      memcpy (optr, mem->ptr + offset, n);
      optr[n] = '\0';
    }
  mu_refcount_unlock (mem->refcount);
  if (nbytes)
    *nbytes = n;
  return 0;
}

int
_stream_memory_write (stream_t stream, const void *iptr, size_t isize,
		      off_t offset, size_t *nbytes)
{
  struct _stream_memory *mem = (struct _stream_memory *)stream;

  mu_refcount_lock (mem->refcount);
  /* Bigger we have to realloc.  */
  if (mem->capacity < (offset + isize))
    {
      /* Realloc by blocks of 512.  */
      int newsize = MU_STREAM_MEMORY_BLOCKSIZE *
	(((offset + isize)/MU_STREAM_MEMORY_BLOCKSIZE) + 1);
      char *tmp =  realloc (mem->ptr, newsize);
      if (tmp == NULL)
	return ENOMEM;
      mem->ptr = tmp;
      mem->size = offset + isize;
      mem->capacity = newsize;
    }

  memcpy (mem->ptr + offset, iptr, isize);
  mu_refcount_unlock (mem->refcount);
  if (nbytes)
    *nbytes = isize;
  return 0;
}

int
_stream_memory_truncate (stream_t stream, off_t len)
{
  struct _stream_memory *mem = (struct _stream_memory *)stream;

  mu_refcount_lock (mem->refcount);
  if (len == 0)
    {
      if (mem->ptr)
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
  mem->capacity = len;
  mem->size = len;
  mu_refcount_unlock (mem->refcount);
  return 0;
}

int
_stream_memory_get_size (stream_t stream, off_t *psize)
{
  struct _stream_memory *mem = (struct _stream_memory *)stream;
  mu_refcount_lock (mem->refcount);
  if (psize)
    *psize = mem->size;
  mu_refcount_unlock (mem->refcount);
  return 0;
}

int
_stream_memory_close (stream_t stream)
{
  struct _stream_memory *mem = (struct _stream_memory *)stream;
  mu_refcount_lock (mem->refcount);
  if (mem->ptr)
    free (mem->ptr);
  mem->ptr = NULL;
  mem->capacity = 0;
  mem->size = 0;
  mu_refcount_unlock (mem->refcount);
  return 0;
}

int
_stream_memory_flush (stream_t stream)
{
  (void)stream;
  return 0;
}

int
_stream_memory_get_fd (stream_t stream, int *pfd)
{
  (void)stream; (void)pfd;
  return MU_ERROR_NOT_SUPPORTED;
}

int
_stream_memory_get_flags (stream_t stream, int *flags)
{
  struct _stream_memory *mem = (struct _stream_memory *)stream;
  if (flags == NULL)
    return MU_ERROR_INVALID_PARAMETER;
  *flags = mem->flags;
  return 0;
}

int
_stream_memory_get_state (stream_t stream, enum stream_state *state)
{
  (void)stream;
  if (state == NULL)
    return MU_ERROR_INVALID_PARAMETER;
  *state = MU_STREAM_NO_STATE;
  return 0;
}

int
_stream_memory_is_seekable (stream_t stream)
{
  (void)stream;
  return 1;
}

int
_stream_memory_tell (stream_t stream, off_t *off)
{
  (void)stream;
  if (off == NULL)
    return MU_ERROR_INVALID_PARAMETER;
  *off = 0;
  return 0;
}

int
_stream_memory_is_open (stream_t stream)
{
  (void)stream;
  return 1;
}

int
_stream_memory_is_readready (stream_t stream, int timeout)
{
  (void)stream;
  (void)timeout;
  return 1;
}

int
_stream_memory_is_writeready (stream_t stream, int timeout)
{
  (void)stream;
  (void)timeout;
  return 1;
}

int
_stream_memory_is_exceptionpending (stream_t stream, int timeout)
{
  (void)stream;
  (void)timeout;
  return 0;
}


int
_stream_memory_open (stream_t stream, const char *filename, int port,
		     int flags)
{
  struct _stream_memory *mem = (struct _stream_memory *)stream;
  int status = 0;

  (void)port; /* Ignored.  */

  mu_refcount_lock (mem->refcount);
  /* Free any previous memory.  */
  if (mem->ptr)
    free (mem->ptr);
  mem->ptr = NULL;
  mem->capacity = 0;
  mem->size = 0;
  mem->flags = flags;
  if (filename)
    {
      struct stat statbuf;
      if (stat (filename, &statbuf) == 0)
	{
	  mem->ptr = calloc (1, statbuf.st_size);
	  if (mem->ptr)
	    {
	      FILE *fp;
	      mem->capacity = statbuf.st_size;
	      mem->size = statbuf.st_size;
	      fp = fopen (filename, "r");
	      if (fp)
		{
		  size_t r = fread (mem->ptr, mem->size, 1, fp);
		  if (r != mem->size)
		    status = MU_ERROR_IO;
		  fclose (fp);
		}
	      else
		status = errno;
	      if (status != 0)
		{
		  free (mem->ptr);
		  mem->ptr = NULL;
		  mem->capacity = 0;
		  mem->size = 0;
		}
	    }
	  else
	    status = MU_ERROR_NO_MEMORY;
	}
      else
	status = MU_ERROR_IO;
    }
  mu_refcount_unlock (mem->refcount);
  return status;
}

static struct _stream_vtable _stream_memory_vtable =
{
  _stream_memory_ref,
  _stream_memory_destroy,

  _stream_memory_open,
  _stream_memory_close,

  _stream_memory_read,
  _stream_memory_readline,
  _stream_memory_write,

  _stream_memory_tell,

  _stream_memory_get_size,
  _stream_memory_truncate,
  _stream_memory_flush,

  _stream_memory_get_fd,
  _stream_memory_get_flags,
  _stream_memory_get_state,

  _stream_memory_is_seekable,
  _stream_memory_is_readready,
  _stream_memory_is_writeready,
  _stream_memory_is_exceptionpending,

  _stream_memory_is_open
};

int
_stream_memory_ctor (struct _stream_memory *mem, size_t capacity)
{
  mu_refcount_create (&mem->refcount);
  if (mem->refcount == NULL)
    return MU_ERROR_NO_MEMORY;
  if (capacity)
    {
      mem->ptr = calloc (1, capacity);
      if (mem->ptr == NULL)
	{
	  mu_refcount_destroy (&mem->refcount);
	  return MU_ERROR_NO_MEMORY;
	}
      mem->capacity = capacity;
    }
  else
    mem->capacity = 0;
  mem->size = 0;
  mem->flags = 0;
  mem->base.vtable = &_stream_memory_vtable;
  return 0;
}

void
_stream_memory_dtor (stream_t stream)
{
  struct _stream_memory *mem = (struct _stream_memory *)stream;
  if (mem)
    {
      mu_refcount_destroy (&mem->refcount);
      mem->ptr = NULL;
      mem->capacity = 0;
      mem->size = 0;
      mem->flags = 0;
    }
}

int
stream_memory_create (stream_t *pstream, size_t capacity)
{
  struct _stream_memory *mem;
  int status;

  if (pstream == NULL)
    return MU_ERROR_INVALID_PARAMETER;

  mem = calloc (1, sizeof (*mem));
  if (mem == NULL)
    return MU_ERROR_NO_MEMORY;

  status = _stream_memory_ctor (mem, capacity);
  if (status != 0)
    {
      free (mem);
      return status;
    }
  mem->base.vtable = &_stream_memory_vtable;
  *pstream = &mem->base;
  return 0;
}
