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
#include <mailutils/cctype.h>
#include <mailutils/cstr.h>

/* A "transcript stream" transparently writes data to and reads data from
   an underlying transport stream, writing each lineful of data to a "log
   stream". Writes to log stream are prefixed with a string indicating
   direction of the data (read/write). Default prefixes are those used in
   RFCs -- "S: ", for data written ("Server"), and "C: ", for data read
   ("Client"). */

#define TRANS_READ     0x1
#define TRANS_WRITE    0x2
#define TRANS_DISABLED 0x4
#define TRANS_IOSTREAM 0x8
#define FLAG_TO_PFX(c) ((c & 0x3) - 1)

static int
word_match (const char *buf, size_t len, int n, const char *word,
	    size_t *pos)
{
  size_t i = 0;
  size_t wl = strlen (word);
  
  for (;; n--)
    {
      /* Skip whitespace separator */
      for (; i < len && mu_isspace (buf[i]); i++)
	;

      if (n == 0)
	break;
      
      /* Skip the argument */
      if (buf[i] == '"')
	{
	  for (i++; i < len && buf[i] != '"'; i++)
	    if (buf[i] == '\'')
	      i++;
	}
      else
	{
	  for (; i < len && !mu_isspace (buf[i]); i++)
	    ;
	}
    }

  if (i + wl <= len &&
      mu_c_strncasecmp (buf + i, word, wl) == 0 &&
      mu_isblank (buf[i + wl]))
    {
      *pos = i + wl;
      return 1;
    }

  return 0;
}

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
	  str->flags &= ~(flag | TRANS_DISABLED);
	}
      
      if (str->flags & TRANS_DISABLED)
	return;
  
      if (str->level == MU_XSCRIPT_PAYLOAD)
	{
	  mu_stream_printf (str->logstr, "(data...)\n");
	  str->flags |= TRANS_DISABLED;
	  return;
	}
  
      p = memchr (buf, '\n', size);
      if (p)
	{
	  len = p - buf;
	  if (p > buf && p[-1] == '\r')
	    len--;

	  if (str->level == MU_XSCRIPT_SECURE)
	    {
	      size_t i;
	      
	      if (word_match (buf, len, 0, "PASS", &i))
		mu_stream_printf (str->logstr, "PASS ***");
	      else if (word_match (buf, len, 1, "LOGIN", &i))
		{
		  /* Skip the whitespace separator */
		  for (; i < len && mu_isspace (buf[i]); i++)
		    ;
		  /* Skip the first argument (presumably the user name) */
		  if (buf[i] == '"')
		    {
		      for (i++; i < len && buf[i] != '"'; i++)
			if (buf[i] == '\'')
			  i++;
		    }
		  else
		    {
		      for (; i < len && !mu_isspace (buf[i]); i++)
			;
		    }
		  mu_stream_write (str->logstr, buf, i, NULL);
		  mu_stream_write (str->logstr, " \"***\"", 6, NULL);
		}
	      else
		mu_stream_write (str->logstr, buf, len, NULL);
	    }
	  else
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

static void
_xscript_event_cb (mu_stream_t str, int ev, unsigned long size, void *ptr)
{
  struct _mu_xscript_stream *sp = (struct _mu_xscript_stream *)str;

  switch (ev)
    {
    case _MU_STR_EVENT_FILLBUF:
      print_transcript (sp, TRANS_READ, ptr, size);
      break;

    case _MU_STR_EVENT_FLUSHBUF:
      print_transcript (sp, TRANS_WRITE, ptr, size);
    }
}

static int
_xscript_read (struct _mu_stream *str, char *buf, size_t bufsize,
	       size_t *pnread)
{
  struct _mu_xscript_stream *sp = (struct _mu_xscript_stream *)str;
  return mu_stream_read (sp->transport, buf, bufsize, pnread);
}

static int
_xscript_readdelim (struct _mu_stream *str, char *buf, size_t bufsize,
		    int delim, size_t *pnread)
{
  struct _mu_xscript_stream *sp = (struct _mu_xscript_stream *)str;
  return mu_stream_readdelim (sp->transport, buf, bufsize, delim, pnread);
}

static int
_xscript_write (struct _mu_stream *str, const char *buf, size_t bufsize,
		  size_t *pnwrite)
{
  struct _mu_xscript_stream *sp = (struct _mu_xscript_stream *)str;
  return mu_stream_write (sp->transport, buf, bufsize, pnwrite);
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
_xscript_ctl (struct _mu_stream *str, int code, int opcode, void *arg)
{
  struct _mu_xscript_stream *sp = (struct _mu_xscript_stream *)str;
  int status = 0;
  
  switch (code)
    {
    case MU_IOCTL_TRANSPORT:
      if (!arg)
	return EINVAL;
      else
	{
	  mu_transport_t *ptrans = arg;
	  switch (opcode)
	    {
	    case MU_IOCTL_OP_GET:
	      ptrans[0] = (mu_transport_t) sp->transport;
	      ptrans[1] = (mu_transport_t) sp->logstr;
	      break;

	    case MU_IOCTL_OP_SET:
	      ptrans = arg;
	      if (ptrans[0])
		sp->transport = (mu_stream_t) ptrans[0];
	      if (ptrans[1])
		sp->logstr = (mu_stream_t) ptrans[1];
	      break;

	    default:
	      return EINVAL;
	    }
	}
      break;

    case MU_IOCTL_SUBSTREAM:
      if (sp->transport &&
          ((status = mu_stream_ioctl (sp->transport, code, opcode, arg)) == 0 ||
            status != ENOSYS))
        return status;
      /* fall through */

    case MU_IOCTL_TOPSTREAM:
      if (!arg)
	return EINVAL;
      switch (opcode)
	{
	case MU_IOCTL_OP_GET:
	  if (!sp->transport)
	    status = ENOSYS;
	  else
	    status = mu_stream_ioctl (sp->transport, code, opcode, arg);
	  if (status == EINVAL || status == ENOSYS)
	    {
	      mu_stream_t *pstr = arg;

	      pstr[0] = sp->transport;
	      mu_stream_ref (pstr[0]);
	      pstr[1] = sp->transport;
	      mu_stream_ref (pstr[1]);
	      status = 0;
	    }
	  break;

	case MU_IOCTL_OP_SET:
	  if (!sp->transport)
	    status = ENOSYS;
	  else
	    status = mu_stream_ioctl (sp->transport, code, opcode, arg);
	  if (status == EINVAL || status == ENOSYS)
	    {
	      mu_stream_t *pstr = arg;
	      mu_stream_t tmp;
	  
	      if (pstr[0] != pstr[1])
		{
		  status = mu_iostream_create (&tmp, pstr[0], pstr[1]);
		  if (status)
		    return status;
		  sp->flags |= TRANS_IOSTREAM;
		}
	      else
		{
		  tmp = pstr[0];
		  mu_stream_ref (tmp);
		  mu_stream_ref (tmp);
		  status = 0;
		}

	      mu_stream_unref (sp->transport);
	      sp->transport = tmp;
	    }
	}
      break;

    case MU_IOCTL_TRANSPORT_BUFFER:
      if (!sp->transport)
	return EINVAL;
      return mu_stream_ioctl (sp->transport, code, opcode, arg);

    case  MU_IOCTL_XSCRIPTSTREAM:
      if (!arg)
	return EINVAL;
      switch (opcode)
	{
	case MU_IOCTL_XSCRIPTSTREAM_LEVEL:
	  {
	    int oldlev = sp->level;
	    sp->level = *(int*)arg;
	    sp->flags &= TRANS_DISABLED;
	    sp->flags |= TRANS_READ | TRANS_WRITE;
	    *(int*)arg = oldlev;
	  }
	  break;
	default:
	  return EINVAL;
	}
      break;
      
    default:
      return mu_stream_ioctl (sp->transport, code, opcode, arg);
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
  sp = (struct _mu_xscript_stream *) _mu_stream_create (sizeof (*sp),
                                                        flags | _MU_STR_OPEN);
  if (!sp)
    return ENOMEM;

  sp->stream.read = _xscript_read; 
  if (transport->readdelim)
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
  sp->stream.event_cb = _xscript_event_cb;
  sp->stream.event_mask = _MU_STR_EVMASK(_MU_STR_EVENT_FILLBUF) |
                          _MU_STR_EVMASK(_MU_STR_EVENT_FLUSHBUF);
  mu_stream_ref (transport);
  mu_stream_ref (logstr);

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
  mu_stream_set_buffer ((mu_stream_t) sp, mu_buffer_line, 0);
  *pref = (mu_stream_t) sp;
  return 0;
}


