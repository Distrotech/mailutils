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

#include <io0.h>

struct _file_stream
{
  FILE		*file;
  int 		offset;
};

static void
_file_destroy (stream_t stream)
{
  struct _file_stream *fs = stream->owner;

  if (fs->file)
    fclose (fs->file);
  free (fs);
}

static int
_file_read (stream_t stream, char *optr, size_t osize,
	    off_t offset, size_t *nbytes)
{
  struct _file_stream *fs = stream->owner;

  if (fs->offset != offset)
    {
      fseek (fs->file, offset, SEEK_SET);
      fs->offset = offset;
    }
  *nbytes = fread (optr, osize, 1, fs->file);
  if (*nbytes == 0)
    {
      if (ferror(fs->file))
	return errno;
    }
  else
    fs->offset += *nbytes;
  return 0;
}

static int
_file_write (stream_t stream, const char *iptr, size_t isize,
	    off_t offset, size_t *nbytes)
{
  struct _file_stream *fs = stream->owner;

  if (fs->offset != offset)
    {
      fseek (fs->file, offset, SEEK_SET);
      fs->offset = offset;
    }
  *nbytes = fwrite (iptr, isize, 1, fs->file);
  if (*nbytes == 0)
    {
      if (ferror (fs->file))
	return errno;
    }
  else
    fs->offset += *nbytes;
  return 0;
}

int
file_stream_create (stream_t *stream, const char *filename, int flags)
{
  struct _file_stream *fs;
  const char *mode;
  int ret;

  if (stream == NULL || filename == NULL)
    return EINVAL;

  if ((fs = calloc(sizeof(struct _file_stream), 1)) == NULL)
    return ENOMEM;

  if (flags & MU_STREAM_RDWR)
    mode = "r+b";
  else if (flags & MU_STREAM_READ)
    mode = "rb";
  else if (flags & MU_STREAM_WRITE)
    mode = "wb";
  else
    return EINVAL;

  if ((fs->file = fopen (filename, mode)) == NULL)
    {
      ret = errno;
      free (fs);
      return ret;
    }
  if ((ret = stream_create (stream, flags, fs)) != 0)
    {
      fclose (fs->file);
      free (fs);
      return ret;
    }

  stream_set_read(*stream, _file_read, fs );
  stream_set_write(*stream, _file_write, fs );
  stream_set_destroy(*stream, _file_destroy, fs );
  return 0;
}

