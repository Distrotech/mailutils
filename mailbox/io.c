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

#include <io0.h>

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

int
stream_create (stream_t *pstream, int flags, void *owner)
{
  stream_t stream;
  if (pstream == NULL || owner == NULL)
    return EINVAL;
  stream = calloc (1, sizeof (*stream));
  if (stream == NULL)
    return ENOMEM;
  stream->owner = owner;
  stream->flags = flags;
  *pstream = stream;
  return 0;
}

void
stream_destroy (stream_t *pstream, void *owner)
{
   if (pstream && *pstream)
    {
      stream_t stream = *pstream;
      if ((stream->flags & MU_STREAM_NO_CHECK) || stream->owner == owner)
	{
	  if (stream->_destroy)
	    stream->_destroy (stream);
	  free (stream);
	}
      *pstream = NULL;
    }
}

int
stream_set_destroy (stream_t stream, void (*_destroy) (stream_t),  void *owner)
{
  if (stream == NULL)
    return EINVAL;

  if (stream->owner != owner)
    return EACCES;

  stream->_destroy = _destroy;
  return 0;
}

int
stream_open (stream_t stream, const char *name, int port, int flags)
{
  if (stream == NULL)
    return EINVAL;
  if (stream->_open)
    return stream->_open (stream, name, port, flags);
  return  0;
}

int
stream_set_open (stream_t stream,
	         int (*_open) (stream_t, const char *, int, int), void *owner)
{
  if (stream == NULL)
    return EINVAL;
  if (owner == stream->owner)
    {
      stream->_open = _open;
      return 0;
    }
  return EACCES;
}

int
stream_close (stream_t stream)
{
  if (stream == NULL)
    return EINVAL;
  if (stream->_close)
    return stream->_close (stream);
  return  0;
}

int
stream_set_close (stream_t stream, int (*_close) (stream_t), void *owner)
{
  if (stream == NULL)
    return EINVAL;
  if (owner == stream->owner)
    {
      stream->_close = _close;
      return 0;
    }
  return EACCES;
}

int
stream_set_fd (stream_t stream, int (*_get_fd) (stream_t, int *), void *owner)
{
  if (stream == NULL)
    return EINVAL;
  if (owner == stream->owner)
    {
      stream->_get_fd = _get_fd;
      return 0;
    }
  return EACCES;
}

int
stream_set_read (stream_t stream, int (*_read)
		 (stream_t, char *, size_t, off_t, size_t *),
		 void *owner)
{
  if (stream == NULL)
    return EINVAL;
  if (owner == stream->owner)
    {
      stream->_read = _read;
      return 0;
    }
  return EACCES;
}

int
stream_set_readline (stream_t stream, int (*_readline)
		 (stream_t, char *, size_t, off_t, size_t *),
		 void *owner)
{
  if (stream == NULL)
    return EINVAL;
  if (owner == stream->owner)
    {
      stream->_readline = _readline;
      return 0;
    }
  return EACCES;
}

int
stream_set_write (stream_t stream, int (*_write)
		  __P ((stream_t, const char *, size_t, off_t, size_t *)),
		  void *owner)
{
  if (stream == NULL)
    return EINVAL;
  if (stream->owner == owner)
    {
      stream->_write = _write;
      return 0;
    }
  return EACCES;
}

int
stream_read (stream_t is, char *buf, size_t count,
	     off_t offset, size_t *pnread)
{
  if (is == NULL || is->_read == NULL)
    return EINVAL;
  return is->_read (is, buf, count, offset, pnread);
}

int
stream_readline (stream_t is, char *buf, size_t count,
		 off_t offset, size_t *pnread)
{
  size_t n, nr = 0;
  char c;
  int status;
  if (is == NULL)
    return EINVAL;
  if (is->_readline != NULL)
    return is->_readline (is, buf, count, offset, pnread);

  /* grossly inefficient hopefully they override this */
  for (n = 1; n < count; n++)
    {
      status = stream_read (is, &c, 1, offset, &nr);
      if (status != 0) /* error */
	return status;
      else if (nr == 1)
	{
	  *buf++ = c;
	  offset++;
	  if (c == '\n') /* newline is stored like fgets() */
	    break;
	}
      else if (nr == 0)
	{
	  if (n == 1) /* EOF, no data read */
	    n = 0;
	  break; /* EOF, some data was read */
	}
    }

  *buf = '\0';
  if (pnread)
    *pnread = n;

  return 0;
}

int
stream_write (stream_t os, const char *buf, size_t count,
	      off_t offset, size_t *pnwrite)
{
  if (os == NULL || os->_write == NULL)
      return EINVAL;
  return os->_write (os, buf, count, offset, pnwrite);
}

int
stream_get_fd (stream_t stream, int *pfd)
{
  if (stream == NULL || stream->_get_fd == NULL)
    return EINVAL;
  return stream->_get_fd (stream, pfd);
}

int
stream_get_flags (stream_t stream, int *pfl)
{
  if (stream == NULL && pfl == NULL )
    return EINVAL;
  *pfl = stream->flags;
  return 0;
}

int
stream_set_flags (stream_t stream, int fl, void *owner)
{
  if (stream == NULL)
    return EINVAL;
  if (stream->owner != owner)
    return EACCES;
  stream->flags = fl;
  return 0;
}

int
stream_size (stream_t stream, off_t *psize)
{
  if (stream == NULL || stream->_size == NULL)
    return EINVAL;
  return stream->_size (stream, psize);
}

int
stream_set_size (stream_t stream, int (*_size)(stream_t, off_t *), void *owner)
{
  if (stream == NULL)
    return EINVAL;
  if (stream->owner != owner)
    return EACCES;
  stream->_size = _size;
  return 0;
}


int
stream_truncate (stream_t stream, off_t len)
{
  if (stream == NULL || stream->_truncate == NULL )
    return EINVAL;

  return stream->_truncate (stream, len);
}

int
stream_set_truncate (stream_t stream, int (*_truncate) (stream_t, off_t),
		     void *owner)
{
  if (stream == NULL)
    return EINVAL;
  if (stream->owner != owner)
    return EACCES;
  stream->_truncate = _truncate;
  return 0;
}

int
stream_flush (stream_t stream)
{
  if (stream == NULL || stream->_flush == NULL)
    return EINVAL;
  return stream->_flush (stream);
}

int
stream_set_flush (stream_t stream, int (*_flush) (stream_t), void *owner)
{
  if (stream == NULL)
    return EINVAL;
  if (stream->owner != owner)
    return EACCES;
  stream->_flush = _flush;
  return 0;
}
