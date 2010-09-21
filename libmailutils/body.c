/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2004, 2005, 2007, 2010 Free Software
   Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library; if not, write to the
   Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301 USA */

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
#include <mailutils/mutil.h>
#include <mailutils/errno.h>
#include <mailutils/sys/stream.h>
#include <mailutils/sys/body.h>

#define BODY_MODIFIED 0x10000

static int _body_flush    (mu_stream_t);
static int _body_read     (mu_stream_t, char *, size_t, size_t *);
static int _body_truncate (mu_stream_t, mu_off_t);
static int _body_size     (mu_stream_t, mu_off_t *);
static int _body_write    (mu_stream_t, const char *, size_t, size_t *);
static int _body_ioctl    (mu_stream_t, int, void *);
static int _body_seek     (mu_stream_t, mu_off_t, mu_off_t *);
static const char *_body_error_string (mu_stream_t, int);

/* Our own defaults for the body.  */
static int _body_get_size   (mu_body_t, size_t *);
static int _body_get_lines  (mu_body_t, size_t *);
static int _body_get_size0  (mu_stream_t, size_t *);
static int _body_get_lines0 (mu_stream_t, size_t *);

int
mu_body_create (mu_body_t *pbody, void *owner)
{
  mu_body_t body;

  if (pbody == NULL)
    return MU_ERR_OUT_PTR_NULL;
  if (owner == NULL)
    return EINVAL;

  body = calloc (1, sizeof (*body));
  if (body == NULL)
    return ENOMEM;

  body->owner = owner;
  *pbody = body;
  return 0;
}

void
mu_body_destroy (mu_body_t *pbody, void *owner)
{
  if (pbody && *pbody)
    {
      mu_body_t body = *pbody;
      if (body->owner == owner)
	{
	  if (body->filename)
	    {
	      /* FIXME: should we do this?  */
	      remove (body->filename);
	      free (body->filename);
	    }

	  if (body->stream)
	    mu_stream_destroy (&body->stream);

	  if (body->fstream)
	    {
	      mu_stream_close (body->fstream);
	      mu_stream_destroy (&body->fstream);
	    }

	  free (body);
	}
      *pbody = NULL;
    }
}

void *
mu_body_get_owner (mu_body_t body)
{
  return (body) ? body->owner : NULL;
}

/* FIXME: not implemented.  */
int
mu_body_is_modified (mu_body_t body)
{
  return (body) ? (body->flags & BODY_MODIFIED) : 0;
}

/* FIXME: not implemented.  */
int
mu_body_clear_modified (mu_body_t body)
{
  if (body)
    body->flags &= ~BODY_MODIFIED;
  return 0;
}

int
mu_body_get_filename (mu_body_t body, char *filename, size_t len, size_t *pn)
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


struct _mu_body_stream
{
  struct _mu_stream stream;
  mu_body_t body;
};

static int
_body_get_stream (mu_body_t body, mu_stream_t *pstream, int ref)
{
  if (body == NULL)
    return EINVAL;
  if (pstream == NULL)
    return MU_ERR_OUT_PTR_NULL;

  if (body->stream == NULL)
    {
      if (body->_get_stream)
	{
	  int status = body->_get_stream (body, &body->stream);
	  if (status)
	    return status;
	}
      else
	{
	  int status;
	  struct _mu_body_stream *str =
	    (struct _mu_body_stream *)
	    _mu_stream_create (sizeof (*str),
			       MU_STREAM_RDWR|MU_STREAM_SEEK);
	  if (!str)
	    return ENOMEM;
	  
	  /* Create the temporary file.  */
	  body->filename = mu_tempname (NULL);
	  status = mu_file_stream_create (&body->fstream, 
					  body->filename, MU_STREAM_RDWR);
	  if (status != 0)
	    return status;
	  mu_stream_set_buffer (body->fstream, mu_buffer_full, 0);
	  status = mu_stream_open (body->fstream);
	  if (status != 0)
	    return status;
	  str->stream.ctl = _body_ioctl;
	  str->stream.read = _body_read;
	  str->stream.write = _body_write;
	  str->stream.truncate = _body_truncate;
	  str->stream.size = _body_size;
	  str->stream.seek = _body_seek;
	  str->stream.flush = _body_flush;
	  str->body = body;
	  body->stream = (mu_stream_t) str;
	  /* Override the defaults.  */
	  body->_lines = _body_get_lines;
	  body->_size = _body_get_size;
	}
    }
  
  if (!ref)
    {
      *pstream = body->stream;
      return 0;
    }
  return mu_streamref_create (pstream, body->stream);
}

int
mu_body_get_stream (mu_body_t body, mu_stream_t *pstream)
{
  /* FIXME: Deprecation warning */
  return _body_get_stream (body, pstream, 0);
}

int
mu_body_get_streamref (mu_body_t body, mu_stream_t *pstream)
{
  return _body_get_stream (body, pstream, 1);
}

int
mu_body_set_stream (mu_body_t body, mu_stream_t stream, void *owner)
{
  if (body == NULL)
   return EINVAL;
  if (body->owner != owner)
    return EACCES;
  /* make sure we destroy the old one if it is owned by the body */
  mu_stream_destroy (&body->stream);
  body->stream = stream;
  body->flags |= BODY_MODIFIED;
  return 0;
}

int
mu_body_set_get_stream (mu_body_t body,
			int (*_getstr) (mu_body_t, mu_stream_t *),
			void *owner)
{
  if (body == NULL)
    return EINVAL;
  if (body->owner != owner)
    return EACCES;
  body->_get_stream = _getstr;
  return 0;
}

int
mu_body_set_lines (mu_body_t body, int (*_lines) (mu_body_t, size_t *),
		   void *owner)
{
  if (body == NULL)
    return EINVAL;
  if (body->owner != owner)
    return EACCES;
  body->_lines = _lines;
  return 0;
}

int
mu_body_lines (mu_body_t body, size_t *plines)
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
mu_body_size (mu_body_t body, size_t *psize)
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
mu_body_set_size (mu_body_t body, int (*_size)(mu_body_t, size_t*) , void *owner)
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
_body_seek (mu_stream_t stream, mu_off_t off, mu_off_t *presult)
{
  struct _mu_body_stream *str = (struct _mu_body_stream*) stream;
  mu_body_t body = str->body;
  return mu_stream_seek (body->fstream, off, MU_SEEK_SET, presult);
}

static const char *
_body_error_string (mu_stream_t stream, int rc)
{
  /* FIXME: How to know if rc was returned by a body->stream? */
  return NULL;
}

static int
_body_ioctl (mu_stream_t stream, int code, void *ptr)
{
  struct _mu_body_stream *str = (struct _mu_body_stream*) stream;
  mu_body_t body = str->body;
  return mu_stream_ioctl (body->fstream, code, ptr);
}

static int
_body_read (mu_stream_t stream, char *buf, size_t size, size_t *pret)
{
  struct _mu_body_stream *str = (struct _mu_body_stream*) stream;
  mu_body_t body = str->body;
  return mu_stream_read (body->fstream, buf, size, pret);
}

static int
_body_write (mu_stream_t stream, const char *buf, size_t size, size_t *pret)
{
  struct _mu_body_stream *str = (struct _mu_body_stream*) stream;
  mu_body_t body = str->body;
  return mu_stream_write (body->fstream, buf, size, pret);
}

static int
_body_truncate (mu_stream_t stream, mu_off_t n)
{
  struct _mu_body_stream *str = (struct _mu_body_stream*) stream;
  mu_body_t body = str->body;
  return mu_stream_truncate (body->fstream, n);
}

static int
_body_size (mu_stream_t stream, mu_off_t *size)
{
  struct _mu_body_stream *str = (struct _mu_body_stream*) stream;
  mu_body_t body = str->body;
  return mu_stream_size (body->fstream, size);
}

static int
_body_flush (mu_stream_t stream)
{
  struct _mu_body_stream *str = (struct _mu_body_stream*) stream;
  mu_body_t body = str->body;
  return mu_stream_flush (body->fstream);
}

/* Default function for the body.  */
static int
_body_get_lines (mu_body_t body, size_t *plines)
{
  return _body_get_lines0 (body->fstream, plines);
}

static int
_body_get_size (mu_body_t body, size_t *psize)
{
  return _body_get_size0 (body->fstream, psize);
}

static int
_body_get_size0 (mu_stream_t stream, size_t *psize)
{
  mu_off_t off = 0;
  int status = mu_stream_size (stream, &off);
  if (psize)
    *psize = off;
  return status;
}

static int
_body_get_lines0 (mu_stream_t stream, size_t *plines)
{
  int status =  mu_stream_flush (stream);
  size_t lines = 0;
  
  if (status == 0)
    {
      char buf[128];
      size_t n = 0;

      status = mu_stream_seek (stream, 0, MU_SEEK_SET, NULL);
      if (status)
	return status;
      while ((status = mu_stream_readline (stream, buf, sizeof buf,
					   &n)) == 0 && n > 0)
	{
	  if (buf[n - 1] == '\n')
	    lines++;
	}
    }
  if (plines)
    *plines = lines;
  return status;
}


