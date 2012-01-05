/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2010-2012 Free Software Foundation, Inc.

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
#include <stdlib.h>
#include <errno.h>
#include <mailutils/types.h>
#include <mailutils/stream.h>
#include <mailutils/errno.h>
#include <mailutils/sys/streamref.h>

#define _MU_STR_ERRMASK (_MU_STR_ERR|_MU_STR_EOF)

static int
streamref_return (struct _mu_streamref *sp, int rc)
{
  if (rc)
    sp->stream.last_err = sp->transport->last_err;
  sp->stream.flags = (sp->stream.flags & ~_MU_STR_ERRMASK) |
                   (sp->transport->flags & _MU_STR_ERRMASK);
  return rc;
}

static int
_streamref_read (struct _mu_stream *str, char *buf, size_t bufsize,
		 size_t *pnread)
{
  struct _mu_streamref *sp = (struct _mu_streamref *)str;
  int rc;
  size_t nread;
  mu_off_t off;
  
  rc = mu_stream_seek (sp->transport, sp->offset, MU_SEEK_SET, &off);
  if (rc == 0)
    {
      if (sp->end)
	{
	  size_t size = sp->end - off + 1;
	  if (size < bufsize)
	    bufsize = size;
	}
      rc = mu_stream_read (sp->transport, buf, bufsize, &nread);
      if (rc == 0)
	{
	  sp->offset += nread;
	  *pnread = nread;
	}
    }
  else if (rc == ESPIPE)
    {
      *pnread = 0;
      mu_stream_clearerr (sp->transport);
      return 0;
    }
  return streamref_return (sp, rc);
}

static int
_streamref_readdelim (struct _mu_stream *str, char *buf, size_t bufsize,
		      int delim, size_t *pnread)
{
  struct _mu_streamref *sp = (struct _mu_streamref *)str;
  int rc;
  size_t nread;
  mu_off_t off;
  
  rc = mu_stream_seek (sp->transport, sp->offset, MU_SEEK_SET, &off);
  if (rc == 0)
    {
      rc = mu_stream_readdelim (sp->transport, buf, bufsize, delim, &nread);
      if (rc == 0)
	{
	  if (sp->end)
	    {
	      size_t size = sp->end - off + 1;
	      if (nread > size)
		nread = size;
	    }
	  sp->offset += nread;
	  *pnread = nread;
	}
    }
  else if (rc == ESPIPE)
    {
      *pnread = 0;
      mu_stream_clearerr (sp->transport);
      return 0;
    }
  return streamref_return (sp, rc);
}

static int
_streamref_write (struct _mu_stream *str, const char *buf, size_t bufsize,
		  size_t *pnwrite)
{
  struct _mu_streamref *sp = (struct _mu_streamref *)str;
  int rc;
  size_t nwrite;
  rc = mu_stream_seek (sp->transport, sp->offset, MU_SEEK_SET, NULL);
  if (rc == 0)
    {
      rc = mu_stream_write (sp->transport, buf, bufsize, &nwrite);
      if (rc == 0)
	{
	  sp->offset += nwrite;
	  *pnwrite = nwrite;
	}
    }
  return streamref_return (sp, rc);
}

static int
_streamref_flush (struct _mu_stream *str)
{
  struct _mu_streamref *sp = (struct _mu_streamref *)str;
  return streamref_return (sp, mu_stream_flush (sp->transport));
}

static int
_streamref_open (struct _mu_stream *str)
{
  struct _mu_streamref *sp = (struct _mu_streamref *)str;
  return streamref_return (sp, mu_stream_open (sp->transport));
}

static int
_streamref_close (struct _mu_stream *str)
{
  struct _mu_streamref *sp = (struct _mu_streamref *)str;
  return streamref_return (sp, mu_stream_close (sp->transport));
}

static void
_streamref_done (struct _mu_stream *str)
{
  struct _mu_streamref *sp = (struct _mu_streamref *)str;
  mu_stream_unref (sp->transport);
}

static int
_streamref_seek (struct _mu_stream *str, mu_off_t off, mu_off_t *ppos)
{
  struct _mu_streamref *sp = (struct _mu_streamref *)str;
  mu_off_t size;
  int rc;
  
  if (sp->end)
    size = sp->end - sp->start + 1;
  else
    {
      rc = mu_stream_size (sp->transport, &size);
      if (rc)
	return streamref_return (sp, rc);
      size -= sp->start;
    }
  
  if (off < 0 || off > size)
    return sp->stream.last_err = ESPIPE;
  rc = mu_stream_seek (sp->transport, sp->start + off, MU_SEEK_SET,
		       &sp->offset);
  if (rc)
    return streamref_return (sp, rc);
  *ppos = sp->offset - sp->start;
  return 0;
}

static int
_streamref_size (struct _mu_stream *str, mu_off_t *psize)
{
  struct _mu_streamref *sp = (struct _mu_streamref *)str;
  mu_off_t size;
  int rc = 0;
  
  if (sp->end)
    size = sp->end - sp->start + 1;
  else
    {
      rc = mu_stream_size (sp->transport, &size);
      if (rc)
	return streamref_return (sp, rc);
      size -= sp->start;
    }
  if (rc == 0)
    *psize = size;
  return rc;
}

static int
_streamref_ctl (struct _mu_stream *str, int code, int opcode, void *arg)
{
  struct _mu_streamref *sp = (struct _mu_streamref *)str;
  
  switch (code)
    {
    case MU_IOCTL_SEEK_LIMITS:
      if (!arg)
	return EINVAL;
      else
	{
	  mu_off_t *lim = arg;

	  switch (opcode)
	    {
	    case MU_IOCTL_OP_GET:
	      lim[0] = sp->start;
	      lim[1] = sp->end;
	      return 0;

	    case MU_IOCTL_OP_SET:
	      sp->start = lim[0];
	      sp->end = lim[1];
	      return 0;

	    default:
	      return EINVAL;
	    }
	}
    }
  return streamref_return (sp, mu_stream_ioctl (sp->transport, code,
						opcode, arg));
}

static int
_streamref_wait (struct _mu_stream *str, int *pflags, struct timeval *tvp)
{
  struct _mu_streamref *sp = (struct _mu_streamref *)str;
  return streamref_return (sp, mu_stream_wait (sp->transport, pflags, tvp));
}

static int
_streamref_truncate (struct _mu_stream *str, mu_off_t size)
{
  struct _mu_streamref *sp = (struct _mu_streamref *)str;
  return streamref_return (sp, mu_stream_truncate (sp->transport, size));
}
  
static int
_streamref_shutdown (struct _mu_stream *str, int how)
{
  struct _mu_streamref *sp = (struct _mu_streamref *)str;
  return streamref_return (sp, mu_stream_shutdown (sp->transport, how));
}

static const char *
_streamref_error_string (struct _mu_stream *str, int rc)
{
  struct _mu_streamref *sp = (struct _mu_streamref *)str;
  const char *p = mu_stream_strerror (sp->transport, rc);
  if (!p)
    p = mu_strerror (rc);
  return p;
}

int
mu_streamref_create_abridged (mu_stream_t *pref, mu_stream_t str,
			      mu_off_t start, mu_off_t end)
{
  int rc;
  mu_off_t off;
  int flags;
  struct _mu_streamref *sp;
  
  rc = mu_stream_seek (str, 0, MU_SEEK_SET, &off);/*FIXME: SEEK_CUR?*/
  if (rc)
    return rc;
  mu_stream_get_flags (str, &flags);
  sp = (struct _mu_streamref *)
         _mu_stream_create (sizeof (*sp), flags | _MU_STR_OPEN);
  if (!sp)
    return ENOMEM;

  mu_stream_ref (str);

  sp->stream.read = _streamref_read; 
  if (str->readdelim)
    sp->stream.readdelim = _streamref_readdelim; 
  sp->stream.write = _streamref_write;
  sp->stream.flush = _streamref_flush;
  sp->stream.open = _streamref_open; 
  sp->stream.close = _streamref_close;
  sp->stream.done = _streamref_done; 
  sp->stream.seek = _streamref_seek; 
  sp->stream.size = _streamref_size; 
  sp->stream.ctl = _streamref_ctl;
  sp->stream.wait = _streamref_wait;
  sp->stream.truncate = _streamref_truncate;
  sp->stream.shutdown = _streamref_shutdown;
  sp->stream.error_string = _streamref_error_string;

  sp->transport = str;
  sp->start = start;
  sp->end = end;
  if (off < start || off > end)
    off = start;
  sp->offset = off;
  *pref = (mu_stream_t) sp;

  mu_stream_set_buffer (*pref, mu_buffer_full, 0);

  return 0;
}

int
mu_streamref_create (mu_stream_t *pref, mu_stream_t str)
{
  return mu_streamref_create_abridged (pref, str, 0, 0);
}

