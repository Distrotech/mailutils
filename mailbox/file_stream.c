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
#include <sys/stat.h>

#include <mailutils/types.h>
#include <mailutils/alloc.h>
#include <mailutils/error.h>
#include <mailutils/errno.h>
#include <mailutils/nls.h>
#include <mailutils/stream.h>
#include <mailutils/sys/stream.h>
#include <mailutils/sys/file_stream.h>
#include <mailutils/mutil.h>

#define MU_FSTR_ERROR_NOREG (MU_ERR_LAST+1)

static int
fd_read (struct _mu_stream *str, char *buf, size_t size, size_t *pret)
{
  struct _mu_file_stream *fstr = (struct _mu_file_stream *) str;
  ssize_t n = read (fstr->fd, buf, size);
  if (n == -1)
    return errno;
  *pret = n;
  return 0;
}

static int
fd_write (struct _mu_stream *str, const char *buf, size_t size, size_t *pret)
{
  struct _mu_file_stream *fstr = (struct _mu_file_stream *) str;
  ssize_t n = write (fstr->fd, (char*) buf, size);
  if (n == -1)
    return errno;
  *pret = n;
  return 0;
}

static int
fd_close (struct _mu_stream *str)
{
  struct _mu_file_stream *fstr = (struct _mu_file_stream *) str;
  if (close (fstr->fd))
    return errno;
  fstr->fd = -1;
  return 0;
}

static int
fd_open (struct _mu_stream *str)
{
  struct _mu_file_stream *fstr = (struct _mu_file_stream *) str;
  int oflg;
  int fd;
  
  if (!fstr->filename)
    return EINVAL;
  if (fstr->fd != -1)
    fd_close (str);

  /* Map the flags to the system equivalent.  */
  if ((fstr->stream.flags & MU_STREAM_RDWR) == MU_STREAM_RDWR)
    oflg = O_RDWR;
  else if (fstr->stream.flags & (MU_STREAM_WRITE|MU_STREAM_APPEND))
    oflg = O_WRONLY;
  else /* default */
    oflg = O_RDONLY;

  if (fstr->stream.flags & MU_STREAM_APPEND)
    oflg |= O_APPEND;

  /* Handle CREAT with care, not to follow symlinks.  */
  if (fstr->stream.flags & MU_STREAM_CREAT)
    {
      /* First see if the file already exists.  */
      fd = open (fstr->filename, oflg);
      if (fd == -1)
	{
	  /* Oops bail out.  */
	  if (errno != ENOENT)
	    return errno;
	  /* Race condition here when creating the file ??.  */
	  fd = open (fstr->filename, oflg|O_CREAT|O_EXCL,
		     0600 | mu_stream_flags_to_mode (fstr->stream.flags, 0));
	}
    }
  else
    fd = open (fstr->filename, oflg);

  if (fd == -1)
    return errno;
  
  if (!(fstr->stream.flags & MU_STREAM_ALLOW_LINKS)
      && (fstr->stream.flags & (MU_STREAM_CREAT | MU_STREAM_RDWR |
				MU_STREAM_APPEND)))
    {
      struct stat fdbuf, filebuf;

      /* The next two stats should never fail.  */
      if (fstat (fd, &fdbuf) == -1
	  || lstat (fstr->filename, &filebuf) == -1)
	{
	  close (fd);
	  return errno;
	}

      /* Now check that: file and fd reference the same file,
	 file only has one link, file is plain file.  */
      if ((fdbuf.st_dev != filebuf.st_dev
	   || fdbuf.st_ino != filebuf.st_ino
	   || fdbuf.st_nlink != 1
	   || filebuf.st_nlink != 1
	   || (fdbuf.st_mode & S_IFMT) != S_IFREG))
	{
	  /* FIXME: Be silent */
	  close (fd);
	  return MU_FSTR_ERROR_NOREG;
	}
    }
  
  if (fd < 0)
    return errno;

  fstr->fd = fd;
  return 0;
}

int
fd_seek (struct _mu_stream *str, mu_off_t off, int whence, mu_off_t *presult)
{ 
  struct _mu_file_stream *fstr = (struct _mu_file_stream *) str;
  off = lseek (fstr->fd, off, whence);
  if (off < 0)
    return errno;
  *presult = off;
  return 0;
}

int
fd_size (struct _mu_stream *str, mu_off_t *psize)
{
  struct _mu_file_stream *fstr = (struct _mu_file_stream *) str;
  struct stat st;
  if (fstat (fstr->fd, &st))
    return errno;
  *psize = st.st_size;
  return 0;
}

void
fd_done (struct _mu_stream *str)
{
  struct _mu_file_stream *fstr = (struct _mu_file_stream *) str;
  if (fstr->fd != -1)
    fd_close (str);
  if (fstr->filename)
    free (fstr->filename);
}

const char *
fd_error_string (struct _mu_stream *str, int rc)
{
  if (rc == MU_FSTR_ERROR_NOREG)
    return _("must be a plain file with one link");
  else
    return mu_strerror (rc);
}

static int
fd_ioctl (struct _mu_stream *str, int code, void *ptr)
{
  struct _mu_file_stream *fstr = (struct _mu_file_stream *) str;
  mu_transport_t (*ptrans)[2];
  
  switch (code)
    {
    case MU_IOCTL_GET_TRANSPORT:
      if (!ptr)
	return EINVAL;
      ptrans = ptr;
      (*ptrans)[0] = (mu_transport_t) fstr->fd;
      (*ptrans)[1] = NULL;
      break;

    default:
      return EINVAL;
    }
  return 0;
}

int
fd_wait (mu_stream_t stream, int *pflags, struct timeval *tvp)
{
  struct _mu_file_stream *fstr = (struct _mu_file_stream *) stream;

  if (fstr->fd == -1)
    return EINVAL;
  return mu_fd_wait (fstr->fd, pflags, tvp);
}

int
fd_truncate (mu_stream_t stream, mu_off_t size)
{
  struct _mu_file_stream *fstr = (struct _mu_file_stream *) stream;
  if (ftruncate (fstr->fd, size))
    return errno;
  return 0;
}

int
_mu_file_stream_create (mu_stream_t *pstream, size_t size,
			char *filename, int flags)
{
  struct _mu_file_stream *str =
    (struct _mu_file_stream *) _mu_stream_create (size,
						  flags | MU_STREAM_SEEK);
  if (!str)
    return ENOMEM;

  str->stream.read = fd_read;
  str->stream.write = fd_write;
  str->stream.open = fd_open;
  str->stream.close = fd_close;
  str->stream.done = fd_done;
  str->stream.seek = fd_seek;
  str->stream.size = fd_size;
  str->stream.ctl = fd_ioctl;
  str->stream.wait = fd_wait;
  str->stream.truncate = fd_truncate;
  str->stream.error_string = fd_error_string;

  str->filename = filename;
  str->fd = -1;
  str->flags = 0;
  *pstream = (mu_stream_t) str;
  return 0;
}

int
mu_file_stream_create (mu_stream_t *pstream, const char *filename, int flags)
{
  int rc;
  char *fname = mu_strdup (filename);
  if (!fname)
    return ENOMEM;
  rc = _mu_file_stream_create (pstream,
			       sizeof (struct _mu_file_stream),
			       fname, flags);
  if (rc)
    free (fname);
  return rc;
}

