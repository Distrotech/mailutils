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
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <mailutils/stream.h>
#include <body0.h>

#define BODY_MODIFIED 0x10000

static int lazy_create    __P ((body_t));
static int _body_flush    __P ((stream_t));
static int _body_get_fd   __P ((stream_t, int *));
static int _body_read     __P ((stream_t, char *, size_t, off_t, size_t *));
static int _body_readline __P ((stream_t, char *, size_t, off_t, size_t *));
static int _body_truncate __P ((stream_t, off_t));
static int _body_size     __P ((stream_t, off_t *));
static int _body_write    __P ((stream_t, const char *, size_t, off_t, size_t *));

/* Our own defaults for the body.  */
static int _body_get_size   __P ((body_t, size_t *));
static int _body_get_lines  __P ((body_t, size_t *));
static int _body_get_size0  __P ((stream_t, size_t *));
static int _body_get_lines0 __P ((stream_t, size_t *));

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
	  if (body->filename)
	    {
	      /* FIXME: should we do this?  */
	      remove (body->filename);
	      free (body->filename);
	    }

	  if (body->stream)
	    stream_destroy (&(body->stream), body);

	  if (body->fstream)
	    {
	      stream_close (body->fstream);
	      stream_destroy (&(body->fstream), NULL);
	    }

	  free (body);
	}
      *pbody = NULL;
    }
}

void *
body_get_owner (body_t body)
{
  return (body) ? body->owner : NULL;
}

/* FIXME: not implemented.  */
int
body_is_modified (body_t body)
{
  (void)body;
  return (body) ? (body->flags & BODY_MODIFIED) : 0;
}

/* FIXME: not implemented.  */
int
body_clear_modified (body_t body)
{
  if (body)
    body->flags &= ~BODY_MODIFIED;
  return 0;
}

int
body_get_filename (body_t body, char *filename, size_t len, size_t *pn)
{
  int n = 0;
  if (body == NULL)
    return EINVAL;
  if (body->filename)
    {
      n = strlen (body->filename);
      if (filename && len > 0)
	{
	  len--; /* Space for the null.  */
	  strncpy (filename, body->filename, len)[len] = '\0';
	}
    }
  if (pn)
    *pn = n;
  return 0;
}

int
body_get_stream (body_t body, stream_t *pstream)
{
  if (body == NULL || pstream == NULL)
    return EINVAL;

  if (body->stream == NULL)
    {
      int fd;
      int status = stream_create (&body->stream, MU_STREAM_RDWR, body);
      if (status != 0)
	return status;
      status = file_stream_create (&body->fstream, body->filename, MU_STREAM_RDWR);
      if (status != 0)
	return status;
      fd = lazy_create (body);
      if (fd == -1)
	return errno;
      status = stream_open (body->fstream);
      close (fd);
      if (status != 0)
	return status;
      stream_set_fd (body->stream, _body_get_fd, body);
      stream_set_read (body->stream, _body_read, body);
      stream_set_readline (body->stream, _body_readline, body);
      stream_set_write (body->stream, _body_write, body);
      stream_set_truncate (body->stream, _body_truncate, body);
      stream_set_size (body->stream, _body_size, body);
      stream_set_flush (body->stream, _body_flush, body);
      /* Override the defaults.  */
      body->_lines = _body_get_lines;
      body->_size = _body_get_size;
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
  body->stream = stream;
  body->flags |= BODY_MODIFIED;
  return 0;
}

int
body_set_lines (body_t body, int (*_lines)__P ((body_t, size_t *)), void *owner)
{
  if (body == NULL)
    return EINVAL;
  if (body->owner != owner)
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
  /* Fall on the stream.  */
  if (body->stream)
    return _body_get_lines0 (body->stream, plines);
  if (plines)
    *plines = 0;
  return 0;
}

int
body_size (body_t body, size_t *psize)
{
  if (body == NULL)
    return EINVAL;
  if (body->_size)
    return body->_size (body, psize);
  /* Fall on the stream.  */
  if (body->stream)
    return _body_get_size0 (body->stream, psize);
  if (psize)
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

/* Stub function for the body stream.  */

static int
_body_get_fd (stream_t stream, int *fd)
{
  body_t body = stream_get_owner (stream);
  return stream_get_fd (body->fstream, fd);
}

static int
_body_read (stream_t stream,  char *buffer, size_t n, off_t off, size_t *pn)
{
  body_t body = stream_get_owner (stream);
  return stream_read (body->fstream, buffer, n, off, pn);
}

static int
_body_readline (stream_t stream, char *buffer, size_t n, off_t off, size_t *pn)
{
  body_t body = stream_get_owner (stream);
  return stream_readline (body->fstream, buffer, n, off, pn);
}

static int
_body_write (stream_t stream, const char *buf, size_t n, off_t off, size_t *pn)
{
  body_t body = stream_get_owner (stream);
  return stream_write (body->fstream, buf, n, off, pn);
}

static int
_body_truncate (stream_t stream, off_t n)
{
  body_t body = stream_get_owner (stream);
  return stream_truncate (body->fstream, n);
}

static int
_body_size (stream_t stream, off_t *size)
{
  body_t body = stream_get_owner (stream);
  return stream_size (body->fstream, size);
}

static int
_body_flush (stream_t stream)
{
  body_t body = stream_get_owner (stream);
  return stream_flush (body->fstream);
}

/* Default function for the body.  */
static int
_body_get_lines (body_t body, size_t *plines)
{
  return _body_get_lines0 (body->fstream, plines);
}

static int
_body_get_size (body_t body, size_t *psize)
{
  return _body_get_size0 (body->fstream, psize);
}

static int
_body_get_size0 (stream_t stream, size_t *psize)
{
  off_t off = 0;
  int status = stream_size (stream, &off);
  if (psize)
    *psize = off;
  return status;
}

static int
_body_get_lines0 (stream_t stream, size_t *plines)
{
  int status =  stream_flush (stream);
  size_t lines = 0;
  if (status == 0)
    {
      char buf[128];
      size_t n = 0;
      off_t off = 0;
      while ((status = stream_readline (stream, buf, sizeof buf,
					off, &n)) == 0 && n > 0)
	{
	  if (buf[n - 1] == '\n')
	    lines++;
	  off += n;
	}
    }
  if (plines)
    *plines = lines;
  return status;
}

#ifndef P_tmpdir
#  define P_tmpdir "/tmp"
#endif

static int
lazy_create (body_t body)
{
  const char *tmpdir = getenv ("TMPDIR");
  int fd;
  if (tmpdir == NULL)
    tmpdir = P_tmpdir;
  body->filename = calloc (strlen (tmpdir) + 1 + /* "muXXXXXX" */ 8 + 1,
			   sizeof (char));
  if (body->filename == NULL)
    return ENOMEM;
  sprintf (body->filename, "%s/muXXXXXX", tmpdir);
#ifdef HAVE_MKSTEMP
  fd = mkstemp (body->filename);
#else
  if (mktemp (body->filename))
    fd = open (body->filename, O_RDWR|O_CREAT|O_EXCL, 0600);
  else
    fd = -1;
#endif
  return fd;
}
