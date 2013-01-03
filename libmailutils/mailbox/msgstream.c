/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2002, 2004-2007, 2009-2012 Free Software
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

#include <config.h>
#include <stdlib.h>

#include <mailutils/types.h>
#include <mailutils/message.h>
#include <mailutils/errno.h>
#include <mailutils/stream.h>
#include <mailutils/header.h>
#include <mailutils/body.h>
#include <mailutils/sys/message.h>
#include <mailutils/sys/stream.h>

/* Message stream */

enum _message_stream_state
  {
    _mss_init,
    _mss_header,
    _mss_body,
    _mss_eof
  };

struct _mu_message_stream
{
  struct _mu_stream stream;
  mu_message_t msg;
  enum _message_stream_state state;
  mu_stream_t transport;
  mu_off_t limit;
};

static int
_check_stream_state (struct _mu_message_stream *str)
{
  int rc = 0;
  
  if (str->transport && mu_stream_eof (str->transport))
    mu_stream_destroy (&str->transport);
  
  switch (str->state)
    {
    case _mss_init:
      if (!str->transport)
	{
	  rc = mu_header_get_streamref (str->msg->header, &str->transport);
	  if (rc == 0)
	    {
	      str->state = _mss_header;
	      rc = mu_stream_seek (str->transport, 0, MU_SEEK_SET, NULL);
	    }
	}
      break;
      
    case _mss_header:
      if (!str->transport)
	{
	  rc = mu_body_get_streamref (str->msg->body, &str->transport);
	  if (rc == 0)
	    {
	      str->state = _mss_body;
	      rc = mu_stream_seek (str->transport, 0, MU_SEEK_SET, NULL);
	    }
	}
      break;
      
    case _mss_body:
      if (!str->transport)
	str->state = _mss_eof;
    case _mss_eof:
      break;
    }
  return rc;
}

static void
_message_stream_done (struct _mu_stream *str)
{
  struct _mu_message_stream *sp = (struct _mu_message_stream *)str;
  mu_stream_destroy (&sp->transport);
}

static int
_message_stream_flush (struct _mu_stream *str)
{
  struct _mu_message_stream *sp = (struct _mu_message_stream *)str;
  int rc = _check_stream_state (sp);
  if (rc)
    return rc;
  return mu_stream_flush (sp->transport);
}
  
static int
_message_stream_size (struct _mu_stream *str, mu_off_t *psize)
{
  struct _mu_message_stream *sp = (struct _mu_message_stream *)str;
  size_t hsize, bsize;
  mu_header_size (sp->msg->header, &hsize);
  mu_body_size (sp->msg->body, &bsize);
  if (psize)
    *psize = hsize + bsize;
  return 0;
}

static int
_message_stream_seek (struct _mu_stream *str, mu_off_t off, mu_off_t *ppos)
{
  struct _mu_message_stream *sp = (struct _mu_message_stream *)str;
  size_t hsize, size;
  int rc;
  
  rc = _check_stream_state (sp);
  if (rc)
    return rc;
  mu_header_size (sp->msg->header, &hsize);
  mu_body_size (sp->msg->body, &size);
  
  if (off < 0 || off >= size + hsize)
    return ESPIPE;

  switch (sp->state)
    {
    case _mss_eof:
      sp->state = _mss_init;
      rc = _check_stream_state (sp);
      if (rc)
	return rc;
      /* fall through */
    case _mss_header:
      if (off < hsize)
	break;
      mu_stream_destroy (&sp->transport);
      rc = _check_stream_state (sp);
      if (rc)
	return rc;
      /* fall through */
    case _mss_body:
      if (off > hsize)
	off -= hsize;	
      else
	{
	  mu_stream_destroy (&sp->transport);
	  sp->state = _mss_init;
	  rc = _check_stream_state (sp);
	  if (rc)
	    return rc;
	}

      break;

    default:
      break;
    }
  rc = mu_stream_seek (sp->transport, off, MU_SEEK_SET, &off);
  if (rc == 0)
    {
      if (sp->state == _mss_body)
	off += hsize;
      *ppos = off;
    }
  return rc;
}

static int
_message_stream_read (struct _mu_stream *str, char *buf, size_t bufsize,
		      size_t *pnread)
{
  struct _mu_message_stream *sp = (struct _mu_message_stream *)str;
  size_t nread = 0;
  int rc;
  
  while (bufsize)
    {
      size_t n;
      rc = _check_stream_state (sp);
      if (rc)
	break;
      if (sp->state == _mss_eof)
	break;
      rc = mu_stream_read (sp->transport, buf, bufsize, &n);
      if (rc)
	break;
      if (n == 0)
	continue;
      nread += n;
      buf += n;
      bufsize -= n;
    }
  *pnread = nread;
  return rc;
}

static int
_message_stream_readdelim (struct _mu_stream *str, char *buf, size_t bufsize,
			   int delim, size_t *pnread)
{
  struct _mu_message_stream *sp = (struct _mu_message_stream *)str;
  size_t nread = 0;
  int rc;
  
  while (bufsize)
    {
      size_t n;
      rc = _check_stream_state (sp);
      if (rc)
	break;
      if (sp->state == _mss_eof)
	break;
      rc = mu_stream_readdelim (sp->transport, buf, bufsize, delim, &n);
      if (rc)
	break;
      if (n == 0)
	continue;
      nread += n;
      if (buf[n-1] == delim)
	break;
      buf += n;
      bufsize -= n;
    }
  *pnread = nread;
  return rc;
}  

#if 0
static int
_message_stream_write (struct _mu_stream *str,
		       const char *buf, size_t bufsize,
		       size_t *pnwritten)
{
  struct _mu_message_stream *sp = (struct _mu_message_stream *)str;
  
  /* FIXME */
}
#endif

static int
_message_stream_create (mu_stream_t *pmsg, mu_message_t msg, int flags)
{
  struct _mu_message_stream *sp;

  sp = (struct _mu_message_stream *) _mu_stream_create (sizeof (*sp),
							flags |
							MU_STREAM_SEEK |
							_MU_STR_OPEN);
  if (!sp)
    return ENOMEM;

  sp->stream.read = _message_stream_read;
  sp->stream.readdelim = _message_stream_readdelim;
  /* FIXME: Write is not defined */
  /*  sp->stream.write = _message_stream_write;*/
  sp->stream.done = _message_stream_done;
  sp->stream.flush = _message_stream_flush;
  sp->stream.seek = _message_stream_seek; 
  sp->stream.size = _message_stream_size;
  sp->state = _mss_init;
  sp->msg = msg;
  *pmsg = (mu_stream_t) sp;
  return 0;
}

int
mu_message_set_stream (mu_message_t msg, mu_stream_t stream, void *owner)
{
  if (msg == NULL)
    return EINVAL;
  if (msg->owner != owner)
    return EACCES;
  mu_stream_destroy (&msg->rawstream);
  mu_stream_destroy (&msg->outstream);
  msg->rawstream = stream;
  msg->flags |= MESSAGE_MODIFIED;
  msg->flags &= ~MESSAGE_INTERNAL_STREAM;
  return 0;
}

static int
mkoutstream (mu_message_t msg)
{
  int status;
  mu_header_t hdr;
  mu_body_t body;

  if (msg->outstream)
    return 0;
  /* FIXME: Kind of a kludge: make sure the message has header
     and body initialized. */
  status = mu_message_get_header (msg, &hdr);
  if (status)
    return status;
  status = mu_message_get_body (msg, &body);
  if (status)
    return status;
	  
  status = _message_stream_create (&msg->outstream, msg, MU_STREAM_RDWR);
  if (status == 0)
    msg->flags |= MESSAGE_INTERNAL_STREAM;
  return status;
}


int
mu_message_get_stream (mu_message_t msg, mu_stream_t *pstream)
{
  int status;

  if (msg == NULL)
    return EINVAL;
  if (pstream == NULL)
    return MU_ERR_OUT_PTR_NULL;

  /* FIXME: Deprecation warning */

  if (msg->rawstream == NULL)
    {
      if (msg->_get_stream)
	{
	  status = msg->_get_stream (msg, &msg->rawstream);
	  if (status)
	    return status;
	}
      else
	{
	  status = mkoutstream (msg);
	  if (status)
	    return status;
	  status = mu_streamref_create (&msg->rawstream, msg->outstream);
	}
    }
  *pstream = msg->rawstream;
  return 0;
}

int
mu_message_get_streamref (mu_message_t msg, mu_stream_t *pstream)
{
  int status = 0;

  if (msg == NULL)
    return EINVAL;
  if (pstream == NULL)
    return MU_ERR_OUT_PTR_NULL;
  
  if (!msg->rawstream)
    {
      if (msg->_get_stream)
	{
	  status = msg->_get_stream (msg, &msg->rawstream);
	  if (status)
	    return status;
	  status = mu_streamref_create (&msg->outstream, msg->rawstream);
	}
      else
	{
	  status = mkoutstream (msg);
	  if (status)
	    return status;
	  status = mu_streamref_create (&msg->rawstream, msg->outstream);
	}
    }
  
  if (status)
    return status;

  if (!msg->outstream ||
      (!(msg->flags & MESSAGE_INTERNAL_STREAM) &&
       mu_message_is_modified (msg)))
    {
      mu_stream_destroy (&msg->outstream);
      status = mkoutstream (msg);
    }
  return mu_streamref_create (pstream, msg->outstream);
}

int
mu_message_set_get_stream (mu_message_t msg,
			   int (*_getstr) (mu_message_t, mu_stream_t *),
			   void *owner)
{
  if (msg == NULL)
    return EINVAL;
  if (msg->owner != owner)
    return EACCES;
  msg->_get_stream = _getstr;
  return 0;
}
