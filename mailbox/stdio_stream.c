/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2009, 2010 Free Software Foundation, Inc.

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

#include <mailutils/types.h>
#include <mailutils/alloc.h>
#include <mailutils/error.h>
#include <mailutils/errno.h>
#include <mailutils/nls.h>
#include <mailutils/stream.h>
#include <mailutils/sys/stream.h>
#include <mailutils/sys/stdio_stream.h>
#include <mailutils/mutil.h>

static int
stdin_read (struct _mu_stream *str, char *buf, size_t size, size_t *pnbytes)
{
  struct _mu_stdio_stream *fs = (struct _mu_stdio_stream *) str;
  int fd = fs->file_stream.fd;
  int status = 0;
  size_t nbytes;
  ssize_t rdbytes;
  
  if (fs->offset < fs->size)
    {
      status = mu_stream_read (fs->cache, buf, size, &nbytes);
      if (status)
	fs->offset += nbytes;
    }
  else if (fs->offset > fs->size)
    {
      size_t left = fs->offset - fs->size + 1;
      char sbuf[1024];
      size_t bufsize;
      char *tmpbuf = malloc (left);
      if (tmpbuf)
	bufsize = left;
      else
	{
	  tmpbuf = sbuf;
	  bufsize = sizeof sbuf;
	}

      while (left > 0)
	{
	  size_t n;
	  
	  rdbytes = read (fd, tmpbuf, bufsize);
	  if (rdbytes < 0)
	    {
	      status = errno;
	      break;
	    }
	  if (rdbytes == 0)
	    {
	      status = EIO; /* FIXME: EOF?? */
	      break;
	    }
	  
	  status = mu_stream_write (fs->cache, tmpbuf, rdbytes, &n);
	  fs->offset += n;
	  left -= n;
	  if (status)
	    break;
	}
      if (tmpbuf != sbuf)
	free (tmpbuf);
      if (status)
	return status;
    }
  
  nbytes = read (fd, buf, size);
  if (nbytes <= 0)
    return EIO;
  else
    {
      status = mu_stream_write (fs->cache, buf, nbytes, NULL);
      if (status)
	return status;
    }
  fs->offset += nbytes;
  fs->size += nbytes;
  
  if (pnbytes)
    *pnbytes = nbytes;
  return status;
}

static int
stdout_write (struct _mu_stream *str, const char *buf, size_t size,
	      size_t *pret)
{
  struct _mu_stdio_stream *fstr = (struct _mu_stdio_stream *) str;
  int n = write (fstr->file_stream.fd, (char*) buf, size);
  if (n == -1)
    return errno;
  fstr->size += n;
  fstr->offset += n;
  return 0;
}

static int
stdio_size (struct _mu_stream *str, off_t *psize)
{
  struct _mu_stdio_stream *fs = (struct _mu_stdio_stream *) str;
  *psize = fs->size;
  return 0;
}

static int
stdio_seek (struct _mu_stream *str, mu_off_t off, int whence, mu_off_t *presult)
{ 
  struct _mu_stdio_stream *fs = (struct _mu_stdio_stream *) str;
  /* FIXME */
  switch (whence)
    {
    case MU_SEEK_SET:
      break;

    case MU_SEEK_CUR:
      off += fs->offset;
      break;

    case MU_SEEK_END:
      off += fs->size;
      break;
    }

  if (off < 0)
    return EINVAL;

  fs->offset = off;
  *presult = fs->offset;
  return 0;
}

int
_mu_stdio_stream_create (mu_stream_t *pstream, size_t size, int flags)
{
  struct _mu_stdio_stream *fs;
  int rc;

  rc = _mu_file_stream_create (pstream, size, NULL, flags);
  if (rc)
    return rc;
  fs = (struct _mu_stdio_stream *) *pstream;
  
  if (flags & MU_STREAM_SEEK)
    {
      if ((rc = mu_memory_stream_create (&fs->cache, MU_STREAM_RDWR))
	  || (rc = mu_stream_open (fs->cache)))
	{
	  mu_stream_destroy ((mu_stream_t*) &fs);
	  return rc;
	}
      fs->file_stream.stream.read = stdin_read;
      fs->file_stream.stream.write = stdout_write;
      fs->file_stream.stream.size = stdio_size;
      fs->file_stream.stream.seek = stdio_seek;
    }
  else
    {
      fs->file_stream.stream.flags &= ~MU_STREAM_SEEK;
      fs->file_stream.stream.seek = NULL;
    }
  fs->file_stream.stream.open = NULL;
  fs->file_stream.fd = -1;
  return 0;
}

int
mu_stdio_stream_create (mu_stream_t *pstream, int fd, int flags)
{
  int rc;
  struct _mu_stdio_stream *fs;

  if (flags & MU_STREAM_SEEK && lseek (fd, 0, 0) == 0)
    flags &= ~MU_STREAM_SEEK;
  switch (fd)
    {
    case MU_STDIN_FD:
      flags |= MU_STREAM_READ;
      break;
      
    case MU_STDOUT_FD:
    case MU_STDERR_FD:
      flags |= MU_STREAM_WRITE;
    }

  rc = _mu_stdio_stream_create (pstream, sizeof (*fs), flags);
  if (rc == 0)
    {
      fs = (struct _mu_stdio_stream *) *pstream;
      fs->file_stream.fd = fd;
    }
  return rc;
}
