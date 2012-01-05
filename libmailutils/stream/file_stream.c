/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2002, 2004-2012 Free Software Foundation, Inc.

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
#if HAVE_TERMIOS_H
# include <termios.h>
#endif

#include <mailutils/types.h>
#include <mailutils/alloc.h>
#include <mailutils/error.h>
#include <mailutils/errno.h>
#include <mailutils/nls.h>
#include <mailutils/stream.h>
#include <mailutils/sys/stream.h>
#include <mailutils/sys/file_stream.h>
#include <mailutils/util.h>

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
  if (fstr->fd != -1)
    {
      if (!(fstr->flags & _MU_FILE_STREAM_FD_BORROWED) && close (fstr->fd))
	return errno;
      fstr->fd = -1;
    }
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
	  fd = open (fstr->filename, oflg|O_CREAT|O_EXCL,
		     0600 | mu_stream_flags_to_mode (fstr->stream.flags, 0));
	}
    }
  else
    fd = open (fstr->filename, oflg);

  if (fd == -1)
    return errno;

  if (lseek (fd, 0, SEEK_SET) == -1)
    str->flags &= ~MU_STREAM_SEEK;
  
  /* Make sure it will be closed */
  fstr->flags &= ~_MU_FILE_STREAM_FD_BORROWED;
  
  fstr->fd = fd;
  return 0;
}

int
fd_seek (struct _mu_stream *str, mu_off_t off, mu_off_t *presult)
{ 
  struct _mu_file_stream *fstr = (struct _mu_file_stream *) str;
  off = lseek (fstr->fd, off, SEEK_SET);
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
  if (fstr->filename && !(fstr->flags & _MU_FILE_STREAM_STATIC_FILENAME))
    free (fstr->filename);
  if (fstr->echo_state)
    free (fstr->echo_state);
}

#ifndef TCSASOFT
# define TCSASOFT 0
#endif

static int
fd_ioctl (struct _mu_stream *str, int code, int opcode, void *ptr)
{
  struct _mu_file_stream *fstr = (struct _mu_file_stream *) str;
  mu_transport_t *ptrans;
  
  switch (code)
    {
    case MU_IOCTL_TRANSPORT:
      if (!ptr)
	return EINVAL;
      switch (opcode)
	{
	case MU_IOCTL_OP_GET:
	  ptrans = ptr;
	  ptrans[0] = (mu_transport_t) fstr->fd;
	  ptrans[1] = NULL;
	  break;

	case MU_IOCTL_OP_SET:
	  ptrans = ptr;
	  fstr->fd = (int) ptrans[0];
	  break;
	}
      break;

    case MU_IOCTL_TRANSPORT_BUFFER:
      if (!ptr)
	return EINVAL;
      else
	{
	  struct mu_buffer_query *qp = ptr;
	  switch (opcode)
	    {
	    case MU_IOCTL_OP_GET:
	      return mu_stream_get_buffer (str, qp);
	    case MU_IOCTL_OP_SET:
	      return mu_stream_set_buffer (str, qp->buftype, qp->bufsize);
	    }
	}
      break;

    case MU_IOCTL_ECHO:
      if (!ptr)
	return EINVAL;
      switch (opcode)
	{
	case MU_IOCTL_OP_GET:
	  *(int*)ptr = fstr->flags & _MU_FILE_STREAM_ECHO_OFF;
	  break;
	case MU_IOCTL_OP_SET:
	  {
	    int status;
	    struct termios t;
	    int state = *(int*)ptr;
#if HAVE_TCGETATTR
	    if (state == 0)
	      {
		if (fstr->flags & _MU_FILE_STREAM_ECHO_OFF)
		  return 0;
		status = tcgetattr (fstr->fd, &t);
		if (status == 0)
		  {
		    fstr->echo_state = malloc (sizeof (t));
		    if (!fstr->echo_state)
		      return ENOMEM;
		    memcpy (fstr->echo_state, &t, sizeof (t));
		    
		    t.c_lflag &= ~(ECHO | ISIG);
		    status = tcsetattr (fstr->fd, TCSAFLUSH | TCSASOFT, &t);
		    if (status == 0)
		      fstr->flags |= _MU_FILE_STREAM_ECHO_OFF;
		  }
		if (status)
		  {
		    status = errno;
		    if (fstr->echo_state)
		      {
			free (fstr->echo_state);
			fstr->echo_state = NULL;
		      }
		  }
	      }
	    else
	      {
		if (!(fstr->flags & _MU_FILE_STREAM_ECHO_OFF))
		  return 0;
		if (tcsetattr (fstr->fd, TCSAFLUSH | TCSASOFT,
			       fstr->echo_state))
		  status = errno;
		else
		  {
		    status = 0;
		    free (fstr->echo_state);
		    fstr->echo_state = NULL;
		    fstr->flags &= ~_MU_FILE_STREAM_ECHO_OFF;
		  }
	      }
	    return status;
#else
	    return ENOSYS;
#endif
	  }
	}
      break;

    case MU_IOCTL_FD:
      if (!ptr)
	return EINVAL;
      switch (opcode)
	{
	case MU_IOCTL_FD_GET_BORROW:
	  *(int*) ptr = !!(fstr->flags & _MU_FILE_STREAM_FD_BORROWED);
	  break;

	case MU_IOCTL_FD_SET_BORROW:
	  if (*(int*)ptr)
	    fstr->flags |= _MU_FILE_STREAM_FD_BORROWED;
	  else
	    fstr->flags &= ~_MU_FILE_STREAM_FD_BORROWED;
	  break;
	}
      break;
	    
    default:
      return ENOSYS;
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

void
_mu_file_stream_setup (struct _mu_file_stream *str)
{
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
}  

int
_mu_file_stream_create (struct _mu_file_stream **pstream, size_t size,
			const char *filename, int fd, int flags)
{
  struct _mu_file_stream *str =
    (struct _mu_file_stream *) _mu_stream_create (size, flags);
  if (!str)
    return ENOMEM;

  _mu_file_stream_setup (str);

  if (filename)
    str->filename = mu_strdup (filename);
  else
    str->filename = NULL;
  str->fd = fd;
  str->flags = 0;
  *pstream = str;
  return 0;
}

int
mu_file_stream_create (mu_stream_t *pstream, const char *filename, int flags)
{
  struct _mu_file_stream *fstr;
  int rc = _mu_file_stream_create (&fstr,
				   sizeof (struct _mu_file_stream),
				   filename, -1,
				   flags | MU_STREAM_SEEK);
  if (rc == 0)
    {
      mu_stream_t stream = (mu_stream_t) fstr;
      mu_stream_set_buffer (stream, mu_buffer_full, 0);
      rc = mu_stream_open (stream);
      if (rc)
	mu_stream_unref (stream);
      else
	*pstream = stream;
    }
  return rc;
}

int
mu_fd_stream_create (mu_stream_t *pstream, char *filename, int fd, int flags)
{
  struct _mu_file_stream *fstr;
  int rc;

  if (flags & MU_STREAM_SEEK)
    {
      if (lseek (fd, 0, SEEK_SET))
	return errno;
    }

  rc = _mu_file_stream_create (&fstr,
			       sizeof (struct _mu_file_stream),
			       filename, fd, flags|_MU_STR_OPEN);
  if (rc == 0)
    {
      mu_stream_t stream = (mu_stream_t) fstr;
      mu_stream_set_buffer (stream, mu_buffer_full, 0);
      *pstream = stream;
    }
  return rc;
}

