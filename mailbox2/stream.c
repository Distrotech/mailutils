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

#include <stdlib.h>
#include <mailutils/error.h>
#include <mailutils/sys/stream.h>

int
(stream_add_ref) (stream_t stream)
{
  if (stream == NULL || stream->vtable == NULL
      || stream->vtable->add_ref == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return stream->vtable->add_ref (stream);
}

int
(stream_release) (stream_t stream)
{
  if (stream == NULL || stream->vtable == NULL
      || stream->vtable->release == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return stream->vtable->release (stream);
}

int
(stream_destroy) (stream_t stream)
{
  if (stream == NULL || stream->vtable == NULL
      || stream->vtable->destroy == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return stream->vtable->destroy (stream);
}

int
(stream_open) (stream_t stream, const char *host, int port, int flag)
{
  if (stream == NULL || stream->vtable == NULL
      || stream->vtable->open == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return stream->vtable->open (stream, host, port, flag);
}

int
(stream_close) (stream_t stream)
{
  if (stream == NULL || stream->vtable == NULL
      || stream->vtable->close == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return stream->vtable->close (stream);
}

int
(stream_read) (stream_t stream, char *buf, size_t buflen, size_t *n)
{
  if (stream == NULL || stream->vtable == NULL
      || stream->vtable->read == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return stream->vtable->read (stream, buf, buflen, n);
}

int
(stream_readline) (stream_t stream, char *buf, size_t buflen, size_t *n)
{
  if (stream == NULL || stream->vtable == NULL
      || stream->vtable->readline == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return stream->vtable->readline (stream, buf, buflen, n);
}

int
(stream_write) (stream_t stream, const char *buf, size_t buflen, size_t *n)
{
  if (stream == NULL || stream->vtable == NULL
      || stream->vtable->write == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return stream->vtable->write (stream, buf, buflen, n);
}

int
(stream_seek) (stream_t stream, off_t off, enum stream_whence whence)
{
  if (stream == NULL || stream->vtable == NULL
      || stream->vtable->seek == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return stream->vtable->seek (stream, off, whence);
}

int
(stream_tell) (stream_t stream, off_t *off)
{
  if (stream == NULL || stream->vtable == NULL
      || stream->vtable->tell == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return stream->vtable->tell (stream, off);
}

int
(stream_get_size) (stream_t stream, off_t *off)
{
  if (stream == NULL || stream->vtable == NULL
      || stream->vtable->get_size == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return stream->vtable->get_size (stream, off);
}

int
(stream_truncate) (stream_t stream, off_t off)
{
  if (stream == NULL || stream->vtable == NULL
      || stream->vtable->truncate == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return stream->vtable->truncate (stream, off);
}

int
(stream_flush) (stream_t stream)
{
  if (stream == NULL || stream->vtable == NULL
      || stream->vtable->flush == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return stream->vtable->flush (stream);
}

int
(stream_get_fd) (stream_t stream, int *fd)
{
  if (stream == NULL || stream->vtable == NULL
      || stream->vtable->get_fd == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return stream->vtable->get_fd (stream, fd);
}

int
(stream_get_flags) (stream_t stream, int *flags)
{
  if (stream == NULL || stream->vtable == NULL
      || stream->vtable->get_flags == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return stream->vtable->get_flags (stream, flags);
}

int
(stream_get_state) (stream_t stream, enum stream_state *state)
{
  if (stream == NULL || stream->vtable == NULL
      || stream->vtable->get_state == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return stream->vtable->get_state (stream, state);
}
