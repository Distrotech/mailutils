/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Library Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with GNU Mailutils; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

#include <mailutils/sys/fstream.h>
#include <mailutils/error.h>


int
_stream_stdio_ref (stream_t stream)
{
  struct _stream_stdio *fs = (struct _stream_stdio *)stream;
  return mu_refcount_inc (fs->refcount);
}

void
_stream_stdio_destroy (stream_t *pstream)
{
  struct _stream_stdio *fs = (struct _stream_stdio *)*pstream;
  if (mu_refcount_dec (fs->refcount) == 0)
    {
      mu_refcount_destroy (&fs->refcount);
      free (fs);
    }
}

int
_stream_stdio_read (stream_t stream, void *optr, size_t osize,
		    off_t offset,size_t *nbytes)
{
  struct _stream_stdio *fs = (struct _stream_stdio *)stream;
  size_t n = 0;
  int err = 0;

  if (!fs->file)
    {
      if (nbytes)
	*nbytes = 0;
      return MU_ERROR_BAD_FILE_DESCRIPTOR;
    }

  if (fs->offset != offset)
    {
      if (fseek (fs->file, offset, SEEK_SET) != 0)
        return errno;
      fs->offset = offset;
    }

  n = fread (optr, 1, osize, fs->file);
  if (n == 0)
    {
      if (ferror(fs->file))
	err = MU_ERROR_IO;
    }

  if (nbytes)
    *nbytes = n;
  return err;
}

int
_stream_stdio_readline (stream_t stream, char *optr, size_t osize,
			off_t offset, size_t *nbytes)
{
  struct _stream_stdio *fs = (struct _stream_stdio *)stream;
  size_t n = 0;
  int err = 0;

  if (!fs->file)
    {
      if (nbytes)
        *nbytes = 0;
      return MU_ERROR_BAD_FILE_DESCRIPTOR;
    }

  if (fs->offset != offset)
    {
      if (fseek (fs->file, offset, SEEK_SET) != 0)
        return errno;
      fs->offset = offset;
    }

  if (fgets (optr, osize, fs->file) != NULL)
    {
      n = strlen (optr);
      /* Oh My!! */
      if (n == 0)
	n++;
      else
	fs->offset += n;
    }
  else
    {
      if (ferror (fs->file))
	err = MU_ERROR_IO;
    }

  if (nbytes)
    *nbytes = n;
  return err;
}

int
_stream_stdio_write (stream_t stream, const void *iptr, size_t isize,
		     off_t offset, size_t *nbytes)
{
  struct _stream_stdio *fs = (struct _stream_stdio *)stream;
  size_t n = 0;
  int err = 0;

  if (!fs->file)
    {
      if (nbytes)
        *nbytes = 0;
      return MU_ERROR_BAD_FILE_DESCRIPTOR;
    }

  if (fs->offset != offset)
    {
      if (fseek (fs->file, offset, SEEK_SET) != 0)
        return errno;
      fs->offset = offset;
    }

  n = fwrite (iptr, 1, isize, fs->file);
  if (n != isize)
    {
      if (feof (fs->file) == 0)
	err = MU_ERROR_IO;
      clearerr(fs->file);
      n = 0;
    }
  else
    fs->offset += n;

  if (nbytes)
    *nbytes = n;
  return err;
}

int
_stream_stdio_truncate (stream_t stream, off_t len)
{
  struct _stream_stdio *fs = (struct _stream_stdio *)stream;
  if (fs->file && ftruncate (fileno (fs->file), len) != 0)
    return MU_ERROR_IO;
  return 0;
}

int
_stream_stdio_get_size (stream_t stream, off_t *psize)
{
  struct _stream_stdio *fs = (struct _stream_stdio *)stream;
  struct stat stbuf;
  stbuf.st_size = 0;
  if (fs->file)
    {
      fflush (fs->file);
      if (fstat (fileno (fs->file), &stbuf) == -1)
	return errno;
    }
  if (psize)
    *psize = stbuf.st_size;
  return 0;
}

int
_stream_stdio_flush (stream_t stream)
{
  struct _stream_stdio *fs = (struct _stream_stdio *)stream;
  if (fs->file)
    return fflush (fs->file);
  return 0;
}

int
_stream_stdio_get_fd (stream_t stream, int *pfd)
{
  struct _stream_stdio *fs = (struct _stream_stdio *)stream;
  int status = 0;
  if (pfd)
    {
      if (fs->file)
	*pfd = fileno (fs->file);
      else
	status = MU_ERROR_BAD_FILE_DESCRIPTOR;
    }
  return status;
}

int
_stream_stdio_is_seekable (stream_t stream)
{
  off_t off;
  return _stream_stdio_tell (stream, &off) == 0;
}

int
_stream_stdio_tell (stream_t stream, off_t *off)
{
  struct _stream_stdio *fs = (struct _stream_stdio *)stream;
  int status = 0;
  if (off == NULL)
    return MU_ERROR_INVALID_PARAMETER;
  if (fs->file)
    {
      *off = ftell (fs->file);
      if (*off == -1)
	{
	  *off = 0;
	  status = errno;
	}
    }
  else
    status = MU_ERROR_BAD_FILE_DESCRIPTOR;
  return status;
}

int
_stream_stdio_get_flags (stream_t stream, int *flags)
{
  struct _stream_stdio *fs = (struct _stream_stdio *)stream;
  if (flags == NULL)
    return MU_ERROR_INVALID_PARAMETER;
  *flags = fs->flags;
  return 0;
}

int
_stream_stdio_get_state (stream_t stream, enum stream_state *state)
{
  (void)stream;
  if (state == NULL)
    return MU_ERROR_INVALID_PARAMETER;
  *state = MU_STREAM_NO_STATE;
  return 0;
}

int
_stream_stdio_is_open (stream_t stream)
{
  struct _stream_stdio *fs = (struct _stream_stdio *)stream;
  return (fs->file) ? 1 : 0;
}

int
_stream_stdio_is_readready (stream_t stream, int timeout)
{
  (void)timeout;
  return _stream_stdio_is_open (stream);
}

int
_stream_stdio_is_writeready (stream_t stream, int timeout)
{
  (void)timeout;
  return _stream_stdio_is_open (stream);
}

int
_stream_stdio_is_exceptionpending (stream_t stream, int timeout)
{
  (void)stream;
  (void)timeout;
  return 0;
}

int
_stream_stdio_close (stream_t stream)
{
  struct _stream_stdio *fs = (struct _stream_stdio *)stream;
  int err = 0;
  if (fs->file)
    {
      if (fclose (fs->file) != 0)
	err = errno;
      fs->file = NULL;
    }
  return err;
}

int
_stream_stdio_open (stream_t stream, const char *filename, int port, int flags)
{
  struct _stream_stdio *fs = (struct _stream_stdio *)stream;
  int flg;
  int fd;
  const char *mode;

  (void)port; /* Ignored.  */

  if (fs->file)
    {
      fclose (fs->file);
      fs->file = NULL;
    }

  /* Map the flags to the system equivalent.  */
  if ((flags & MU_STREAM_WRITE) && !(flags & MU_STREAM_READ))
    flg = O_WRONLY;
  else if (flags & MU_STREAM_RDWR)
    flg = O_RDWR;
  else /* default */
    flg = O_RDONLY;

  /* Local folders should not block it is local disk ???
     We simply ignore the O_NONBLOCK flag
     But take care of the APPEND.  */
  if (flags & MU_STREAM_APPEND)
    flg |= O_APPEND;

  /* Handle CREAT with care, not to follow symlinks.  */
  if (flags & MU_STREAM_CREAT)
    {
      /* First see if the file already exists.  */
      fd = open(filename, flg);
      if (fd == -1)
	{
	  /* Oops bail out.  */
	  if (errno != ENOENT)
	    return errno;
	  /* Race condition here when creating the file ??.  */
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

  /* We have to make sure that We did not open
     a symlink. From Casper D. in bugtraq.  */
  if ((flg & MU_STREAM_CREAT) ||
      (flg & MU_STREAM_RDWR) ||
      (flg & MU_STREAM_WRITE))
    {
      struct stat fdbuf, filebuf;

      /* The next two stats should never fail.  */
      if (fstat(fd, &fdbuf) == -1)
	return errno;
      if (lstat(filename, &filebuf) == -1)
	return errno;

      /* Now check that: file and fd reference the same file,
	 file only has one link, file is plain file.  */
      if (fdbuf.st_dev != filebuf.st_dev
	  || fdbuf.st_ino != filebuf.st_ino
	  || fdbuf.st_nlink != 1
	  || filebuf.st_nlink != 1
	  || !S_ISREG(fdbuf.st_mode))
	{
	  /*mu_error ("%s must be a plain file with one link\n", filename); */
	  close (fd);
	  return EINVAL;
	}
    }
  /* We use FILE * object.  */
  if (flags & MU_STREAM_APPEND)
    mode = "a";
  else if (flags & MU_STREAM_RDWR)
    mode = "r+b";
  else if (flags & MU_STREAM_WRITE)
    mode = "wb";
  else /* Default readonly.  */
    mode = "rb";

  fs->file = fdopen (fd, mode);
  if (fs->file == NULL)
    {
      int ret = errno;
      free (fs);
      return ret;
    }
  fs->flags = flags;
  return 0;
}

static struct _stream_vtable _stream_stdio_vtable =
{
  _stream_stdio_ref,
  _stream_stdio_destroy,

  _stream_stdio_open,
  _stream_stdio_close,

  _stream_stdio_read,
  _stream_stdio_readline,
  _stream_stdio_write,

  _stream_stdio_tell,

  _stream_stdio_get_size,
  _stream_stdio_truncate,
  _stream_stdio_flush,

  _stream_stdio_get_fd,
  _stream_stdio_get_flags,
  _stream_stdio_get_state,

  _stream_stdio_is_seekable,
  _stream_stdio_is_readready,
  _stream_stdio_is_writeready,
  _stream_stdio_is_exceptionpending,

  _stream_stdio_is_open
};

int
_stream_stdio_ctor (struct _stream_stdio *fs, FILE *fp)
{
  mu_refcount_create (&fs->refcount);
  if (fs->refcount == NULL)
    return MU_ERROR_NO_MEMORY ;
  fs->file = fp;
  fs->flags = 0;
  fs->base.vtable = &_stream_stdio_vtable;
  return 0;
}

void
_stream_stdio_dtor (stream_t stream)
{
  struct _stream_stdio *fs = (struct _stream_stdio *)stream;
  if (fs)
    {
      mu_refcount_destroy (&fs->refcount);
      /* We may leak if they did not close the FILE * */
      fs->file = NULL;
    }
}

int
stream_file_create (stream_t *pstream)
{
  struct _stream_stdio *fs;

  if (pstream == NULL)
    return MU_ERROR_INVALID_PARAMETER;

  fs = calloc (1, sizeof *fs);
  if (fs == NULL)
    return MU_ERROR_NO_MEMORY ;

  mu_refcount_create (&fs->refcount);
  if (fs->refcount == NULL)
    {
      free (fs);
      return MU_ERROR_NO_MEMORY ;
    }
  fs->file = NULL;
  fs->flags = 0;
  fs->base.vtable = &_stream_stdio_vtable;
  *pstream = &fs->base;
  return 0;
}

int
stream_stdio_create (stream_t *pstream, FILE *fp)
{
  struct _stream_stdio *fs;

  if (pstream == NULL)
    return MU_ERROR_INVALID_PARAMETER;

  fs = calloc (1, sizeof *fs);
  if (fs == NULL)
    return MU_ERROR_NO_MEMORY ;

  mu_refcount_create (&fs->refcount);
  if (fs->refcount == NULL)
    {
      free (fs);
      return MU_ERROR_NO_MEMORY ;
    }
  fs->file = fp;
  fs->flags = 0;
  fs->base.vtable = &_stream_stdio_vtable;
  *pstream = &fs->base;
  return 0;
}
