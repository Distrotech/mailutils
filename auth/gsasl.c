/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA  */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#ifdef WITH_GSASL

#include <stdlib.h>
#include <string.h>
#include <mailutils/argp.h>
#include <mailutils/error.h>
#include <mailutils/mu_auth.h>
#include <mailutils/nls.h>
#include <mailutils/stream.h>

#include <gsasl.h>

struct _gsasl_stream {
  Gsasl_session_ctx *sess_ctx; /* Context */
  int last_err;        /* Last Gsasl error code */
  
  stream_t stream;     /* Underlying stream */
  size_t offset;       /* Current offset */

  char *buffer;        /* Line buffer */
  size_t size;         /* Allocated size */
  size_t level;        /* Current filling level */
};

static void
_gsasl_destroy (stream_t stream)
{
  struct _gsasl_stream *s = stream_get_owner (stream);
  stream_destroy (&s->stream, stream_get_owner (s->stream));
  free (s->buffer);
  s->buffer = NULL;
}

#define buffer_drop(s) s->level = 0

int
buffer_grow (struct _gsasl_stream *s, char *ptr, size_t size)
{
  if (!s->buffer)
    {
      s->buffer = malloc (size);
      s->size = size;
      s->level = 0;
    }
  else if (s->size - s->level < size)
    {
      size_t newsize = s->size + size;
      s->buffer = realloc (s->buffer, newsize);
      if (s->buffer)
	s->size = newsize;
    }

  if (!s->buffer)
    return ENOMEM;
  
  memcpy (s->buffer + s->level, ptr, size);
  s->level += size;
  return 0;
}

static int
_gsasl_readline (stream_t stream, char *optr, size_t osize,
		 off_t offset, size_t *nbytes)
{
  struct _gsasl_stream *s = stream_get_owner (stream);
  int rc;
  size_t len, sz;
  char *bufp;
  
  if (s->level)
    {
      len = s->level > osize ? osize : s->level;
      memcpy (optr, s->buffer, len);
      if (s->level > len)
	{
	  memmove (s->buffer, s->buffer + len, s->level - len);
	  s->level -= len;
	}
      if (nbytes)
	*nbytes = len;
      return 0;
    }
  
  do
    {
      char buf[80];
      size_t sz;
      
      rc = stream_readline (s->stream, buf, sizeof (buf), s->offset, &sz);
      if (rc)
	return rc;

      s->offset += sz;
      
      rc = buffer_grow (s, buf, sz);
      if (rc)
	return rc;

      rc = gsasl_decode (s->sess_ctx, s->buffer, s->level, NULL, &len);
    }
  while (rc == GSASL_NEEDS_MORE);

  if (rc != GSASL_OK)
    {
      s->last_err = rc;
      return EIO;
    }
      
  bufp = malloc (len + 1);
  if (!bufp)
    return ENOMEM;
  rc = gsasl_decode (s->sess_ctx, s->buffer, s->level, bufp, &len);
  if (rc != GSASL_OK)
    {
      s->last_err = rc;
      return EIO;
    }
  bufp[len++] = '\0';

  sz = len > osize ? osize : len;
  
  if (len > osize)
    {
      memcpy (optr, bufp, osize);
      buffer_drop (s);
      buffer_grow (s, bufp + osize, len - osize);
      len = osize;
    }
  else
    memcpy (optr, bufp, len);

  if (nbytes)
    *nbytes = len;
  
  free (bufp);

  return 0;
}

static int
_gsasl_write (stream_t stream, const char *iptr, size_t isize,
	      off_t offset, size_t *nbytes)
{
  int rc;
  struct _gsasl_stream *s = stream_get_owner (stream);
  
  rc = buffer_grow (s, iptr, isize);
  if (rc)
    return rc;
  
  if (s->level >= 2
      && s->buffer[s->level - 2] == '\r'
      && s->buffer[s->level - 1] == '\n')
    {
      size_t len, wrsize;
      char *buf = NULL;
      
      gsasl_encode (s->sess_ctx, s->buffer, s->level, NULL, &len);
      buf = malloc (len);
      if (!buf)
	return ENOMEM;

      gsasl_encode (s->sess_ctx, s->buffer, s->level, buf, &len);
      rc = stream_write (s->stream, buf, len, s->offset, &wrsize);
      free (buf);
      if (rc)
	return rc;
      s->offset += wrsize;

      if (nbytes)
	*nbytes = wrsize;
      
      s->level = 0;
  } 
  return 0;
}

static int
_gsasl_flush (stream_t stream)
{
  struct _gsasl_stream *s = stream_get_owner (stream);
  stream_flush (s->stream);
  return 0;
}

static int
_gsasl_close (stream_t stream)
{
  struct _gsasl_stream *s = stream_get_owner (stream);
  stream_close (s->stream);
  if (s->sess_ctx)
    gsasl_server_finish (s->sess_ctx);
  return 0;
}

static int
_gsasl_open (stream_t stream)
{
  struct _gsasl_stream *s = stream_get_owner (stream);
  return 0;
}

int
_gsasl_strerror (stream_t stream, const char **pstr)
{
  struct _gsasl_stream *s = stream_get_owner (stream);
  *pstr = gsasl_strerror (s->last_err);
  return 0;
}


int
gsasl_stream_create (stream_t *stream, stream_t ins,
		     Gsasl_session_ctx *ctx, int flags)
{
  struct _gsasl_stream *s;

  if (stream == NULL)
    return EINVAL;

  if ((flags & ~(MU_STREAM_READ|MU_STREAM_WRITE))
      || (flags & (MU_STREAM_READ|MU_STREAM_WRITE)) ==
          (MU_STREAM_READ|MU_STREAM_WRITE))
    return EINVAL;
  
  s = calloc (1, sizeof (*s));
  if (s == NULL)
    return ENOMEM;

  s->stream = ins;
  s->sess_ctx = ctx;

  stream_set_open (*stream, _gsasl_open, s);
  stream_set_close (*stream, _gsasl_close, s);
  stream_set_flush (*stream, _gsasl_flush, s);
  stream_set_destroy (*stream, _gsasl_destroy, s);
  stream_set_strerror (*stream, _gsasl_strerror, s);

  if (flags & MU_STREAM_READ)
    stream_set_readline (*stream, _gsasl_readline, s);
  else
    stream_set_write (*stream, _gsasl_write, s);
    
  return 0;
}
  
#endif
