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
  size_t n;

  if (fs->offset != offset)
    {
      fseek (fs->file, offset, SEEK_SET);
      fs->offset = offset;
    }

  n = fread (optr, sizeof(char), osize, fs->file);

  if (n == 0)
    {
      if (ferror(fs->file))
	return errno;
    }
  else
    fs->offset += n;

  if (nbytes)
    *nbytes = n;
  return 0;
}

static int
_file_readline (stream_t stream, char *optr, size_t osize,
		off_t offset, size_t *nbytes)
{
  struct _file_stream *fs = stream->owner;
  size_t n = 0;
  int err = 0;

  if (fs->offset != offset)
    {
      fseek (fs->file, offset, SEEK_SET);
      fs->offset = offset;
    }

  if (fgets (optr, osize, fs->file) != NULL)
    {
      char *tmp = optr;
      while (*tmp) tmp++; /* strlen(optr) */
      n = tmp - optr;
      fs->offset += n;
    }
  else
    {
      if (ferror (fs->file))
	err = errno;
    }

  if (nbytes)
    *nbytes = n;
  return err;
}

static int
_file_write (stream_t stream, const char *iptr, size_t isize,
	    off_t offset, size_t *nbytes)
{
  struct _file_stream *fs = stream->owner;
  size_t n;

  if (fs->offset != offset)
    {
      fseek (fs->file, offset, SEEK_SET);
      fs->offset = offset;
    }

  n = fwrite (iptr, sizeof(char), isize, fs->file);

  if (*nbytes == 0)
    {
      if (ferror (fs->file))
	return errno;
    }
  else
    fs->offset += *nbytes;

  if (nbytes)
    *nbytes = n;
  return 0;
}

static int
_file_truncate (stream_t stream, off_t len)
{
  struct _file_stream *fs = stream->owner;
  if (ftruncate (fileno(fs->file), len) == -1)
    return errno;
  return 0;
}

static int
_file_size (stream_t stream, off_t *psize)
{
  struct _file_stream *fs = stream->owner;
  struct stat stbuf;
  if (fstat(fileno(fs->file), &stbuf) == -1)
    return errno;
  if (psize)
    *psize = stbuf.st_size;
  return 0;
}

static int
_file_flush (stream_t stream)
{
  struct _file_stream *fs = stream->owner;
  return fflush (fs->file);
}

static int
_file_get_fd (stream_t stream, int *pfd)
{
  struct _file_stream *fs = stream->owner;
  if (pfd)
    *pfd = fileno (fs->file);
  return 0;
}

static int
_file_open (stream_t stream, const char *filename, int port, int flags)
{
  struct _file_stream *fs = stream->owner;
  int flg;
  int fd;
  const char *mode;

  (void)port; /* shutup gcc */
  /* map the flags to the system equivalent */
  if (flags & MU_STREAM_WRITE)
    flg = O_WRONLY;
  else if (flags & MU_STREAM_RDWR)
    flg = O_RDWR;
  else /* default */
    flg = O_RDONLY;

  /* local folders should not block it is local disk ???
   * We simply ignore the O_NONBLOCK flag
   * But take care of the APPEND.
   */
  if (flags & MU_STREAM_APPEND)
    flg |= O_APPEND;

  /* handle CREAT with care, not to follow symlinks */
  if (flags & MU_STREAM_CREAT)
    {
      /* first see if the file already exists */
      fd = open(filename, flg);
      if (fd == -1)
	{
	  /* oops bail out */
	  if (errno != ENOENT)
	    return errno;
	  /* Race condition here when creating the file ?? */
	  fd = open(filename, flg|O_CREAT|O_EXCL, 0600);
	  if (fd < 0)
	    return errno;
	}
    }
  else
    {
      fd = open (filename, flg);
      if (fd < 0)
        return errno;
    }

  /* we have to make sure that We did not open
   * a symlink. From Casper D. in bugtraq.
   */
  if ((flg & MU_STREAM_CREAT) ||
      (flg & MU_STREAM_RDWR) ||
      (flg & MU_STREAM_WRITE))
    {
      struct stat fdbuf, filebuf;

      /* the next two stats should never fail */
      if (fstat(fd, &fdbuf) == -1)
	return errno;
      if (lstat(filename, &filebuf) == -1)
	return errno;

      /* Now check that: file and fd reference the same file,
	 file only has one link, file is plain file */
      if (fdbuf.st_dev != filebuf.st_dev ||
	  fdbuf.st_ino != filebuf.st_ino ||
	  fdbuf.st_nlink != 1 ||
	  filebuf.st_nlink != 1 ||
	  (fdbuf.st_mode & S_IFMT) != S_IFREG) {
	fprintf(stderr,"%s must be a plain file with one link\n", filename);
	return EINVAL;
      }
    }
  /* we use FILE * object */
  if (flags & MU_STREAM_APPEND)
    mode = "a";
  else if (flags & MU_STREAM_RDWR)
    mode = "r+b";
  else if (flags & MU_STREAM_WRITE)
    mode = "wb";
  else /* default readonly*/
    mode = "rb";

  fs->file = fopen (filename, mode);
  if (fs->file == NULL)
    {
      int ret = errno;
      free (fs);
      return ret;
    }
#if BUFSIZ <= 1024
  {
    char *iobuffer;
    iobuffer = malloc (8192);
    if (iobuffer != NULL)
      if (setvbuf (file, iobuffer, _IOFBF, 8192) != 0)
	free (iobuffer);
  }
#endif
  stream_set_flags (stream, flags |MU_STREAM_NO_CHECK, fs);
  return 0;
}

int
file_stream_create (stream_t *stream)
{
  struct _file_stream *fs;
  int ret;

  if (stream == NULL)
    return EINVAL;

  fs = calloc (1, sizeof (struct _file_stream));
  if (fs == NULL)
    return ENOMEM;

  ret = stream_create (stream, MU_STREAM_NO_CHECK, fs);
  if (ret != 0)
    {
      fclose (fs->file);
      free (fs);
      return ret;
    }

  stream_set_open (*stream, _file_open, fs);
  stream_set_fd (*stream, _file_get_fd, fs);
  stream_set_read (*stream, _file_read, fs);
  stream_set_readline (*stream, _file_readline, fs);
  stream_set_write (*stream, _file_write, fs);
  stream_set_truncate (*stream, _file_truncate, fs);
  stream_set_size (*stream, _file_size, fs);
  stream_set_flush (*stream, _file_flush, fs);
  stream_set_destroy (*stream, _file_destroy, fs);
  return 0;
}
