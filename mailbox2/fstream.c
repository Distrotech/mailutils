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


static int
_fs_ref (stream_t stream)
{
  struct _fs *fs = (struct _fs *)stream;
  return mu_refcount_inc (fs->refcount);
}

static void
_fs_destroy (stream_t *pstream)
{
  struct _fs *fs = (struct _fs *)*pstream;
  if (mu_refcount_dec (fs->refcount) == 0)
    {
      if (fs->file)
	fclose (fs->file);
      mu_refcount_destroy (&fs->refcount);
      free (fs);
    }
}

static int
_fs_read (stream_t stream, void *optr, size_t osize, size_t *nbytes)
{
  struct _fs *fs = (struct _fs *)stream;
  size_t n = 0;
  int err = 0;

  if (fs->file)
    {
      n = fread (optr, 1, osize, fs->file);
      if (n == 0)
	{
	  if (ferror(fs->file))
	    err = MU_ERROR_IO;
	}
    }

  if (nbytes)
    *nbytes = n;
  return err;
}

static int
_fs_readline (stream_t stream, char *optr, size_t osize, size_t *nbytes)
{
  struct _fs *fs = (struct _fs *)stream;
  size_t n = 0;
  int err = 0;

  if (fs->file)
    {
      if (fgets (optr, osize, fs->file) != NULL)
	{
	  n = strlen (optr);
	}
      else
	{
	  if (ferror (fs->file))
	    err = MU_ERROR_IO;
	}
    }

  if (nbytes)
    *nbytes = n;
  return err;
}

static int
_fs_write (stream_t stream, const void *iptr, size_t isize, size_t *nbytes)
{
  struct _fs *fs = (struct _fs *)stream;
  size_t n = 0;
  int err = 0;

  if (fs->file)
    {
      n = fwrite (iptr, 1, isize, fs->file);
      if (n != isize)
	{
	  if (feof (fs->file) == 0)
	    err = MU_ERROR_IO;
	  clearerr(fs->file);
	  n = 0;
	}
    }

  if (nbytes)
    *nbytes = n;
  return err;
}

static int
_fs_truncate (stream_t stream, off_t len)
{
  struct _fs *fs = (struct _fs *)stream;
  if (fs->file && ftruncate (fileno (fs->file), len) != 0)
    return MU_ERROR_IO;
  return 0;
}

static int
_fs_get_size (stream_t stream, off_t *psize)
{
  struct _fs *fs = (struct _fs *)stream;
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

static int
_fs_flush (stream_t stream)
{
  struct _fs *fs = (struct _fs *)stream;
  if (fs->file)
    return fflush (fs->file);
  return 0;
}

static int
_fs_get_fd (stream_t stream, int *pfd)
{
  struct _fs *fs = (struct _fs *)stream;
  int status = 0;
  if (pfd)
    {
      if (fs->file)
	*pfd = fileno (fs->file);
      else
	status = MU_ERROR_INVALID_PARAMETER;
    }
  return status;
}

static int
_fs_seek (stream_t stream, off_t off, enum stream_whence whence)
{
  struct _fs *fs = (struct _fs *)stream;
  int err = 0;
  if (fs->file)
    {
      if (whence == MU_STREAM_WHENCE_SET)
	err = fseek (fs->file, off, SEEK_SET);
      else if (whence == MU_STREAM_WHENCE_CUR)
	err = fseek (fs->file, off, SEEK_CUR);
      else if (whence == MU_STREAM_WHENCE_END)
	err = fseek (fs->file, off, SEEK_END);
      else
	err = MU_ERROR_INVALID_PARAMETER;
      if (err == -1)
	err = errno;
    }
  return err;
}

static int
_fs_tell (stream_t stream, off_t *off)
{
  struct _fs *fs = (struct _fs *)stream;
  if (off == NULL)
    return MU_ERROR_INVALID_PARAMETER;
  *off = (fs->file) ? ftell (fs->file) : 0;
  return 0;
}

static int
_fs_get_flags (stream_t stream, int *flags)
{
  struct _fs *fs = (struct _fs *)stream;
  if (flags == NULL)
    return MU_ERROR_INVALID_PARAMETER;
  *flags = fs->flags;
  return 0;
}

static int
_fs_get_state (stream_t stream, enum stream_state *state)
{
  (void)stream;
  if (state == NULL)
    return MU_ERROR_INVALID_PARAMETER;
  *state = MU_STREAM_NO_STATE;
  return 0;
}

static int
_fs_is_open (stream_t stream)
{
  struct _fs *fs = (struct _fs *)stream;
  return (fs->file) ? 1 : 0;
}

static int
_fs_is_readready (stream_t stream, int timeout)
{
  (void)timeout;
  return _fs_is_open (stream);
}

static int
_fs_is_writeready (stream_t stream, int timeout)
{
  (void)timeout;
  return _fs_is_open (stream);
}

static int
_fs_is_exceptionpending (stream_t stream, int timeout)
{
  (void)stream;
  (void)timeout;
  return 0;
}


static int
_fs_close (stream_t stream)
{
  struct _fs *fs = (struct _fs *)stream;
  int err = 0;
  if (fs->file)
    {
      if (fclose (fs->file) != 0)
	err = errno;
      fs->file = NULL;
    }
  return err;
}

static int
_fs_open (stream_t stream, const char *filename, int port, int flags)
{
  struct _fs *fs = (struct _fs *)stream;
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
  if (flags & MU_STREAM_WRITE && flags & MU_STREAM_READ)
    return EINVAL;
  else if (flags & MU_STREAM_WRITE)
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
	  mu_error ("%s must be a plain file with one link\n", filename);
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

static struct _stream_vtable _fs_vtable =
{
  _fs_ref,
  _fs_destroy,

  _fs_open,
  _fs_close,

  _fs_read,
  _fs_readline,
  _fs_write,

  _fs_seek,
  _fs_tell,

  _fs_get_size,
  _fs_truncate,
  _fs_flush,

  _fs_get_fd,
  _fs_get_flags,
  _fs_get_state,

  _fs_is_readready,
  _fs_is_writeready,
  _fs_is_exceptionpending,

  _fs_is_open
};

int
stream_file_create (stream_t *pstream)
{
  struct _fs *fs;

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
  fs->base.vtable = &_fs_vtable;
  *pstream = &fs->base;
  return 0;
}

int
stream_stdio_create (stream_t *pstream, FILE *fp)
{
  struct _fs *fs;

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
  fs->base.vtable = &_fs_vtable;
  *pstream = &fs->base;
  return 0;
}
