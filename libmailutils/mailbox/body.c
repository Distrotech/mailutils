/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2001, 2004-2005, 2007, 2010-2012 Free Software
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
   Public License along with this library.  If not, see 
   <http://www.gnu.org/licenses/>. */

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
#include <mailutils/util.h>
#include <mailutils/errno.h>
#include <mailutils/sys/stream.h>
#include <mailutils/sys/body.h>

#define BODY_MODIFIED         0x10000

static int body_get_stream (mu_body_t body, mu_stream_t *pstream, int ref);


struct _mu_body_stream
{
  struct _mu_stream stream;
  mu_body_t body;
};

/* Body stream.  */
#define BODY_RDONLY 0
#define BODY_RDWR 1

static int
init_tmp_stream (mu_body_t body)
{
  int rc;
  mu_off_t off;

  rc = mu_stream_seek (body->rawstream, 0, MU_SEEK_CUR, &off);
  if (rc)
    return rc;

  rc = mu_stream_seek (body->rawstream, 0, MU_SEEK_SET, NULL);
  if (rc)
    return rc;
  
  rc = mu_stream_copy (body->fstream, body->rawstream, 0, NULL);
  if (rc)
    return rc;
  
  mu_stream_seek (body->rawstream, off, MU_SEEK_SET, NULL);

  return mu_stream_seek (body->fstream, off, MU_SEEK_SET, NULL);
}


static int
body_stream_transport (mu_stream_t stream, int mode, mu_stream_t *pstr)
{
  struct _mu_body_stream *str = (struct _mu_body_stream*) stream;
  mu_body_t body = str->body;
  
  if (!body->rawstream && body->_get_stream)
    {
      int status = body->_get_stream (body, &body->rawstream);
      if (status)
	return status;
    }
  if (mode == BODY_RDWR || !body->rawstream)
    {
      /* Create the temporary file.  */
      if (!body->fstream)
	{
	  int rc;
	  
	  rc = mu_temp_file_stream_create (&body->fstream, NULL, 0);
	  if (rc)
	    return rc;
	  mu_stream_set_buffer (body->fstream, mu_buffer_full, 0);
	  if (body->rawstream)
	    {
	      rc = init_tmp_stream (body);
	      if (rc)
		{
		  mu_stream_destroy (&body->fstream);
		  return rc;
		}
	    }
	}
      body->flags |= BODY_MODIFIED;
    }
  *pstr = body->fstream ? body->fstream : body->rawstream;
  return 0;
}
    
static int
bstr_close (struct _mu_stream *stream)
{
  struct _mu_body_stream *str = (struct _mu_body_stream*) stream;
  mu_body_t body = str->body;
  mu_stream_close (body->rawstream);
  mu_stream_close (body->fstream);
  return 0;
}

void
bstr_done (struct _mu_stream *stream)
{
  struct _mu_body_stream *str = (struct _mu_body_stream*) stream;
  mu_body_t body = str->body;
  mu_stream_destroy (&body->rawstream);
  mu_stream_destroy (&body->fstream);  
}

static int
bstr_seek (mu_stream_t stream, mu_off_t off, mu_off_t *presult)
{
  mu_stream_t transport;
  int rc;
  
  rc = body_stream_transport (stream, BODY_RDONLY, &transport);
  if (rc)
    return rc;
  
  return mu_stream_seek (transport, off, MU_SEEK_SET, presult);
}

static int
bstr_ioctl (mu_stream_t stream, int code, int opcode, void *ptr)
{
  mu_stream_t transport;
  int rc;

  /* FIXME: always RDONLY, is it? */
  rc = body_stream_transport (stream, BODY_RDONLY, &transport);
  if (rc)
    return rc;
  return mu_stream_ioctl (transport, code, opcode, ptr);
}

static int
bstr_read (mu_stream_t stream, char *buf, size_t size, size_t *pret)
{
  mu_stream_t transport;
  int rc;
  
  rc = body_stream_transport (stream, BODY_RDONLY, &transport);
  if (rc)
    return rc;
  return mu_stream_read (transport, buf, size, pret);
}

static int
bstr_write (mu_stream_t stream, const char *buf, size_t size, size_t *pret)
{
  mu_stream_t transport;
  int rc;
  
  rc = body_stream_transport (stream, BODY_RDWR, &transport);
  if (rc)
    return rc;
  return mu_stream_write (transport, buf, size, pret);
}

static int
bstr_truncate (mu_stream_t stream, mu_off_t n)
{
  mu_stream_t transport;
  int rc;
  
  rc = body_stream_transport (stream, BODY_RDWR, &transport);
  if (rc)
    return rc;
  return mu_stream_truncate (transport, n);
}

static int
bstr_size (mu_stream_t stream, mu_off_t *size)
{
  mu_stream_t transport;
  int rc;
  
  rc = body_stream_transport (stream, BODY_RDONLY, &transport);
  if (rc)
    return rc;
  return mu_stream_size (transport, size);
}

static int
bstr_flush (mu_stream_t stream)
{
  mu_stream_t transport;
  int rc;
  
  rc = body_stream_transport (stream, BODY_RDONLY, &transport);
  if (rc)
    return rc;
  return mu_stream_flush (transport);
}

/* Default function for the body.  */
static int
bstr_get_lines (mu_body_t body, size_t *plines)
{
  mu_stream_t stream, transport;
  int status;
  size_t lines = 0;
  mu_off_t off;

  status = body_get_stream (body, &stream, 0);
  if (status)
    return status;

  status = body_stream_transport (body->stream, BODY_RDONLY, &transport);
  if (status)
    return status;
  
  status = mu_stream_flush (transport);
  if (status)
    return status;
  status = mu_stream_seek (transport, 0, MU_SEEK_CUR, &off);
  if (status == 0)
    {
      char buf[128];
      size_t n = 0;

      status = mu_stream_seek (transport, 0, MU_SEEK_SET, NULL);
      if (status)
	return status;
      while ((status = mu_stream_readline (transport, buf, sizeof buf,
					   &n)) == 0 && n > 0)
	{
	  if (buf[n - 1] == '\n')
	    lines++;
	}
      mu_stream_seek (transport, off, MU_SEEK_SET, NULL);
    }
  if (plines)
    *plines = lines;
  return status;
}

static int
bstr_get_size0 (mu_stream_t stream, size_t *psize)
{
  mu_off_t off = 0;
  int status = mu_stream_size (stream, &off);
  if (psize)
    *psize = off;
  return status;
}

static int
bstr_get_size (mu_body_t body, size_t *psize)
{
  mu_stream_t transport;
  int rc;
  
  rc = body_stream_transport (body->stream, BODY_RDONLY, &transport);
  if (rc)
    return rc;
  return bstr_get_size0 (transport, psize);
}


static int
body_stream_create (mu_body_t body)
{
  struct _mu_body_stream *str =
    (struct _mu_body_stream *)
	    _mu_stream_create (sizeof (*str),
			       MU_STREAM_RDWR|MU_STREAM_SEEK|_MU_STR_OPEN);
  if (!str)
    return ENOMEM;
	  
  str->stream.ctl = bstr_ioctl;
  str->stream.read = bstr_read;
  str->stream.write = bstr_write;
  str->stream.truncate = bstr_truncate;
  str->stream.size = bstr_size;
  str->stream.seek = bstr_seek;
  str->stream.flush = bstr_flush;
  str->stream.close = bstr_close;
  str->stream.done = bstr_done;
  str->body = body;
  body->stream = (mu_stream_t) str;
  /* Override the defaults.  */
  body->_lines = bstr_get_lines;
  body->_size = bstr_get_size;

  body->stream = (mu_stream_t) str;
  return 0;
}

static int
body_get_stream (mu_body_t body, mu_stream_t *pstream, int ref)
{
  if (body == NULL)
    return EINVAL;
  if (pstream == NULL)
    return MU_ERR_OUT_PTR_NULL;

  if (body->stream == NULL)
    {
      int status = body_stream_create (body);
      if (status)
	return status;
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
  return body_get_stream (body, pstream, 0);
}

int
mu_body_get_streamref (mu_body_t body, mu_stream_t *pstream)
{
  return body_get_stream (body, pstream, 1);
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
  mu_stream_destroy (&body->rawstream);
  body->rawstream = stream;
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
  return bstr_get_lines (body, plines);
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
    return bstr_get_size0 (body->stream, psize);
  if (psize)
    *psize = 0;
  return 0;
}

int
mu_body_set_size (mu_body_t body, int (*_size)(mu_body_t, size_t*),
		  void *owner)
{
  if (body == NULL)
    return EINVAL;
  if (body->owner != owner)
    return EACCES;
  body->_size = _size;
  return 0;
}


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
	  mu_stream_destroy (&body->rawstream);
	  mu_stream_destroy (&body->stream);
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

int
mu_body_is_modified (mu_body_t body)
{
  return (body) ? (body->flags & BODY_MODIFIED) : 0;
}

int
mu_body_clear_modified (mu_body_t body)
{
  if (body)
    body->flags &= ~BODY_MODIFIED;
  return 0;
}



