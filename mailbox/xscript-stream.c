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
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <mailutils/types.h>
#include <mailutils/alloc.h>
#include <mailutils/errno.h>

#include <mailutils/nls.h>
#include <mailutils/stream.h>
#include <mailutils/sys/stream.h>
#include <mailutils/sys/xscript-stream.h>

/* A "transcript stream" transparently writes data to and reads data from
   an underlying transport stream, writing each lineful of data to a "log
   stream". Writes to log stream are prefixed with a string indicating
   direction of the data (read/write). Default prefixes are those used in
   RFCs -- "S: ", for data written ("Server"), and "C: ", for data read
   ("Client"). */

#define TRANS_READ 0x1
#define TRANS_WRITE 0x2
#define FLAG_TO_PFX(c) ((c) - 1)

static void
print_transcript (struct _mu_xscript_stream *str, int flag,
		  const char *buf, size_t size)
{
    while (size)
      {
	const char *p;
	size_t len;
	
	if (str->flags & flag)
	  {
	    mu_stream_write (str->logstr,
			     str->prefix[FLAG_TO_PFX(flag)],
			     strlen (str->prefix[FLAG_TO_PFX (flag)]),
			     NULL);
	    str->flags &= ~flag;
	  }
	p = memchr (buf, '\n', size);
	if (p)
	  {
	    len = p - buf;
	    if (p > buf && p[-1] == '\r')
	      len--;
	    mu_stream_write (str->logstr, buf, len, NULL);
	    mu_stream_write (str->logstr, "\n", 1, NULL);
	    str->flags |= flag;

	    len = p - buf + 1;
	    buf = p + 1;
	    size -= len;
	  }
	else
	  {
	    mu_stream_write (str->logstr, buf, size, NULL);
	    break;
	  }
      }
}

static int
_xscript_read (struct _mu_stream *str, char *buf, size_t bufsize,
	       size_t *pnread)
{
  struct _mu_xscript_stream *sp = (struct _mu_xscript_stream *)str;
  size_t nbytes;
  int rc = mu_stream_read (sp->transport, buf, bufsize, &nbytes);

  if (rc == 0)
    {
      print_transcript (sp, TRANS_READ, buf, nbytes);
      if (pnread)
	*pnread = nbytes;
    }
  return rc;
}

static int
_xscript_readdelim (struct _mu_stream *str, char *buf, size_t bufsize,
		    int delim, size_t *pnread)
{
  struct _mu_xscript_stream *sp = (struct _mu_xscript_stream *)str;
  size_t nread;
  int rc = mu_stream_readdelim (sp->transport, buf, bufsize, delim, &nread);
  if (rc == 0)
    {
      print_transcript (sp, TRANS_READ, buf, nread);
      if (pnread)
	*pnread = nread;
    }
  return rc;
}

static int
_xscript_write (struct _mu_stream *str, const char *buf, size_t bufsize,
		  size_t *pnwrite)
{
  struct _mu_xscript_stream *sp = (struct _mu_xscript_stream *)str;
  int rc = mu_stream_write (sp->transport, buf, bufsize, pnwrite);

  if (rc == 0)
    print_transcript (sp, TRANS_WRITE, buf, pnwrite ? *pnwrite : bufsize);
  return rc;
}

static int
_xscript_flush (struct _mu_stream *str)
{
  struct _mu_xscript_stream *sp = (struct _mu_xscript_stream *)str;
  return mu_stream_flush (sp->transport);
}

static int
_xscript_open (struct _mu_stream *str)
{
  struct _mu_xscript_stream *sp = (struct _mu_xscript_stream *)str;
  return mu_stream_open (sp->transport);
}

static int
_xscript_close (struct _mu_stream *str)
{
  struct _mu_xscript_stream *sp = (struct _mu_xscript_stream *)str;
  return mu_stream_close (sp->transport);
}

static void
_xscript_done (struct _mu_stream *str)
{
  struct _mu_xscript_stream *sp = (struct _mu_xscript_stream *)str;
  free (sp->prefix[0]);
  free (sp->prefix[1]);
  mu_stream_unref (sp->transport);
  mu_stream_unref (sp->logstr);
}

static int
_xscript_seek (struct _mu_stream *str, mu_off_t off, mu_off_t *ppos)
{
  struct _mu_xscript_stream *sp = (struct _mu_xscript_stream *)str;
  return mu_stream_seek (sp->transport, off, MU_SEEK_SET, ppos);
}

static int
_xscript_size (struct _mu_stream *str, mu_off_t *psize)
{
  struct _mu_xscript_stream *sp = (struct _mu_xscript_stream *)str;
  return mu_stream_size (sp->transport, psize);
}

static int
_xscript_ctl (struct _mu_stream *str, int op, void *arg)
{
  struct _mu_xscript_stream *sp = (struct _mu_xscript_stream *)str;
  mu_transport_t *ptrans;
  int status = 0;
  
  switch (op)
    {
    case MU_IOCTL_GET_TRANSPORT:
      if (!arg)
	return EINVAL;
      ptrans = arg;
      ptrans[0] = (mu_transport_t) sp->transport;
      ptrans[1] = (mu_transport_t) sp->logstr;
      break;

    case MU_IOCTL_SET_TRANSPORT:
      if (!arg)
	return EINVAL;
      ptrans = arg;
      if (ptrans[0])
	sp->transport = (mu_stream_t) ptrans[0];
      if (ptrans[1])
	sp->logstr = (mu_stream_t) ptrans[1];
      break;

    case MU_IOCTL_SWAP_STREAM:
      if (!arg)
	return EINVAL;
      if (!sp->transport)
	status = ENOSYS;
      else
	status = mu_stream_ioctl (sp->transport, op, arg);
      if (status == EINVAL || status == ENOSYS)
	{
	  mu_stream_t *pstr = arg;
	  mu_stream_t tmp;

	  if (pstr[0] != pstr[1])
	    return EINVAL; /* FIXME */
	  tmp = pstr[0];
	  pstr[0] = sp->transport;
	  pstr[1] = sp->transport;
	  sp->transport = tmp;
	  if (!(str->flags & MU_STREAM_AUTOCLOSE))
	    {
	      if (pstr[0])
		mu_stream_unref (pstr[0]);
	      if (tmp)
		mu_stream_ref (tmp);
	    }
	  if (tmp)
	    mu_stream_ref (tmp);
	  status = 0;
	}
      break;
      
    default:
      return mu_stream_ioctl (sp->transport, op, arg);
    }
  return status;
}

static int
_xscript_wait (struct _mu_stream *str, int *pflags, struct timeval *tvp)
{
  struct _mu_xscript_stream *sp = (struct _mu_xscript_stream *)str;
  return mu_stream_wait (sp->transport, pflags, tvp);
}

static int
_xscript_truncate (struct _mu_stream *str, mu_off_t size)
{
  struct _mu_xscript_stream *sp = (struct _mu_xscript_stream *)str;
  return mu_stream_truncate (sp->transport, size);
}

static int
_xscript_shutdown (struct _mu_stream *str, int how)
{
  struct _mu_xscript_stream *sp = (struct _mu_xscript_stream *)str;
  return mu_stream_shutdown (sp->transport, how);
}

static const char *
_xscript_error_string (struct _mu_stream *str, int rc)
{
  struct _mu_xscript_stream *sp = (struct _mu_xscript_stream *)str;
  const char *p = mu_stream_strerror (sp->transport, rc);
  if (!p)
    p = mu_strerror (rc);
  return p;
}

const char *default_prefix[2] = {
    "C: ", "S: "
};

int
mu_xscript_stream_create(mu_stream_t *pref, mu_stream_t transport,
			 mu_stream_t logstr,
			 const char *prefix[])
{
  int flags;
  struct _mu_xscript_stream *sp;

  mu_stream_get_flags (transport, &flags);
  sp = (struct _mu_xscript_stream *) _mu_stream_create (sizeof (*sp), flags);
  if (!sp)
    return ENOMEM;

  sp->stream.read = _xscript_read; 
  sp->stream.readdelim = _xscript_readdelim; 
  sp->stream.write = _xscript_write;
  sp->stream.flush = _xscript_flush;
  sp->stream.open = _xscript_open; 
  sp->stream.close = _xscript_close;
  sp->stream.done = _xscript_done; 
  sp->stream.seek = _xscript_seek; 
  sp->stream.size = _xscript_size; 
  sp->stream.ctl = _xscript_ctl;
  sp->stream.wait = _xscript_wait;
  sp->stream.truncate = _xscript_truncate;
  sp->stream.shutdown = _xscript_shutdown;
  sp->stream.error_string = _xscript_error_string;

  if (!(flags & MU_STREAM_AUTOCLOSE))
    {
      mu_stream_ref (transport);
      mu_stream_ref (logstr);
    }
  sp->transport = transport;
  sp->logstr = logstr;
  
  sp->flags = TRANS_READ | TRANS_WRITE;
  if (prefix)
    {
      sp->prefix[0] = strdup(prefix[0] ? prefix[0] : default_prefix[0]);
      sp->prefix[1] = strdup(prefix[1] ? prefix[1] : default_prefix[1]);
    }
  else
    {
      sp->prefix[0] = strdup(default_prefix[0]);
      sp->prefix[1] = strdup(default_prefix[1]);
    }

  if (sp->prefix[0] == NULL || sp->prefix[1] == 0)
    {
      free (sp->prefix[0]);
      free (sp->prefix[1]);
      free (sp);
      return ENOMEM;
    }
  mu_stream_set_buffer ((mu_stream_t) sp, mu_buffer_line, 1024);
  *pref = (mu_stream_t) sp;
  return 0;
}


