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


#include <body0.h>
#include <io0.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

static int body_read (stream_t is, char *buf, size_t buflen,
			 off_t off, size_t *pnread );
static int body_write (stream_t os, const char *buf, size_t buflen,
			  off_t off, size_t *pnwrite);
static FILE *lazy_create (void);
static int body_get_fd (stream_t stream, int *pfd);

int
body_create (body_t *pbody, void *owner)
{
  body_t body;

  if (pbody == NULL || owner == NULL)
    return EINVAL;

  body = calloc (1, sizeof (*body));
  if (body == NULL)
    return ENOMEM;

  body->owner = owner;
  *pbody = body;
  return 0;
}

void
body_destroy (body_t *pbody, void *owner)
{
  if (pbody && *pbody)
    {
      body_t body = *pbody;
      if (body->owner == owner)
	{
	  if (body->file)
	    {
	      fclose (body->file);
	      body->file = NULL;
	    }
	  free (body->filename); body->filename = NULL;
	  stream_destroy (&(body->stream), body);
	}
      *pbody = NULL;
    }
}

int
body_get_stream (body_t body, stream_t *pstream)
{
  if (body == NULL || pstream == NULL)
    return EINVAL;

  if (body->stream == NULL)
    {
      stream_t stream;
      int status;
      /* lazy floating body it is created when
       * doing the first body_write call
       */
      status = stream_create (&stream, MU_STREAM_RDWR, body);
      if (status != 0)
	return status;
      stream_set_read (stream, body_read, body);
      stream_set_write (stream, body_write, body);
      stream_set_fd (stream, body_get_fd, body);
      body->stream = stream;
    }

  *pstream = body->stream;
  return 0;
}

int
body_set_stream (body_t body, stream_t stream, void *owner)
{
  if (body == NULL)
   return EINVAL;
  if (body->owner != owner)
    return EACCES;
  /* make sure we destroy the old one if it is own by the body */
  stream_destroy (&(body->stream), body);
  if (body->file)
    {
      fclose (body->file);
      body->file = NULL;
    }
  body->stream = stream;
  return 0;
}

int
body_set_lines (body_t body, int (*_lines)(body_t, size_t *), void *owner)
{
  if (body == NULL)
    return EINVAL;
  if (body->owner == owner)
    return EACCES;
  body->_lines = _lines;
  return 0;
}

int
body_lines (body_t body, size_t *plines)
{
  if (body == NULL)
    return EINVAL;
  if (body->_lines)
    return body->_lines (body, plines);
  if (plines)
    *plines = 0;
  return 0;
}

int
body_size (body_t body, size_t *psize)
{
  if (body == NULL)
    return EINVAL;

  /* Check to see if they want to doit themselves,
   * it was probably not a floating message */
  if (body->_size)
    return body->_size (body, psize);

  /* ok we should handle this */
  if (body->file)
    {
      struct stat st;
      if (fstat (fileno (body->file), &st) == -1)
	return errno;
      if (psize)
	*psize = st.st_size;
    }
  else if (body->filename)
    {
      struct stat st;
      if (stat (body->filename, &st) == -1)
	return errno;
      if (psize)
	*psize = st.st_size;
    }
  else if (psize)
    *psize = 0;

  return 0;
}

int
body_set_size (body_t body, int (*_size)(body_t, size_t*) , void *owner)
{
  if (body == NULL)
    return EINVAL;
  if (body->owner != owner)
    return EACCES;
  body->_size = _size;
  return 0;
}

static int
body_read (stream_t is, char *buf, size_t buflen,
	      off_t off, size_t *pnread )
{
  body_t body;
  size_t nread = 0;

  if (is == NULL || (body = is->owner) == NULL)
    return EINVAL;

  /* check if they want to read from a file */
  if (body->file == NULL && body->filename)
    {
      /* try read only, we don't want to
       * handle nasty security issues here.
       */
      body->file = fopen (body->filename, "r");
      if (body->file == NULL)
	{
	  if (pnread)
	    *pnread = 0;
	  return errno;
	}
    }

  if (body->file)
    {
      /* should we check the error of fseek for some handlers
       * like socket where fseek () will fail.
       * FIXME: Alternative is to check fseeck and errno == EBADF
       * if not a seekable stream.
       */
      if (fseek (body->file, off, SEEK_SET) == -1)
	return errno;

      nread = fread (buf, sizeof (char), buflen, body->file);
      if (nread == 0)
	{
	  if (ferror (body->file))
	    return errno;
	  /* clear the error for feof() */
	  clearerr (body->file);
	}
    }

  if (pnread)
    *pnread = nread;
  return 0;
}

static int
body_write (stream_t os, const char *buf, size_t buflen,
	       off_t off, size_t *pnwrite)
{
  body_t body;
  size_t nwrite = 0;

  if (os == NULL || (body = os->owner) == NULL)
    return EINVAL;

  /* FIXME: security issues, Refuse to write to an unknow file */
  if (body->file == NULL && body->filename)
    return EINVAL;

  /* Probably being lazy, then create a body for the stream */
  if (body->file == NULL)
    {
      body->file = lazy_create ();
      if (body->file == NULL)
	return errno;
    }

  /* should we check the error of fseek for some handlers
   * like socket where it does not make sense.
   * FIXME: Alternative is to check fseeck and errno == EBADF
   * if not a seekable stream.
   */
  if (fseek (body->file, off, SEEK_SET) == -1)
    return errno;

  nwrite = fwrite (buf, sizeof (char), buflen, body->file);
  if (nwrite == 0)
    {
      if (ferror (body->file))
	return errno;
      /* clear the error for feof() */
      clearerr (body->file);
    }

  if (pnwrite)
    *pnwrite = nwrite;
  return 0;
}

static int
body_get_fd (stream_t stream, int *pfd)
{
  body_t body;

  if (stream == NULL || (body = stream->owner) == NULL)
    return EINVAL;

  /* Probably being lazy, then create a body for the stream */
  if (body->file == NULL)
    {
      body->file = lazy_create ();
      if (body->file == 0)
        return errno;
    }

  if (pfd)
    *pfd = fileno (body->file);
  return 0;
}

static FILE *
lazy_create ()
{
  FILE *file;
#ifdef HAVE_MKSTEMP
  char tmpbuf[L_tmpnam + 1];
  int fd;

  if (tmpnam (tmpbuf) == NULL ||
      (fd = mkstemp (tmpbuf)) == -1 ||
      (file = fdopen(fd, "w+")) == NULL)
    return NULL;
  (void)remove(tmpbuf);
#else
  file = tmpfile ();
  /* make sure the mode is right */
  if (file)
    fchmod (fileno (file), 0600);
#endif
  return file;
}
