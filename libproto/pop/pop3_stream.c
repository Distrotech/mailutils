/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003, 2007, 2010-2012 Free Software Foundation, Inc.

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

#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <mailutils/filter.h>
#include <mailutils/stream.h>
#include <mailutils/sys/stream.h>
#include <mailutils/sys/pop3.h>

/* Implementation of the stream for TOP and RETR.  */

#define _POP3F_DONE  0x01
#define _POP3F_CHBUF 0x02

struct mu_pop3_stream
{
  struct _mu_stream stream;
  mu_pop3_t pop3;
  int flags;
  struct mu_buffer_query oldbuf;
};

/* Called on _MU_STR_EVENT_SETFLAG */
static void
_pop3_event_cb (mu_stream_t str, int ev, unsigned long flags,
		void *ptr MU_ARG_UNUSED)
{
  if (flags & _MU_STR_EOF)
    {
      mu_transport_t trans[2];

      if (mu_stream_ioctl (str, MU_IOCTL_TRANSPORT, MU_IOCTL_OP_GET, 
                           trans) == 0)
	{
	  struct mu_pop3_stream *sp = (struct mu_pop3_stream *) trans[0];
	  _mu_pop3_xscript_level (sp->pop3, MU_XSCRIPT_NORMAL);
	  sp->pop3->state = MU_POP3_NO_STATE;

	  if (sp->flags & _POP3F_CHBUF)
	    {
	      mu_stream_ioctl (sp->pop3->carrier,
			       MU_IOCTL_TRANSPORT_BUFFER, MU_IOCTL_OP_SET,
			       &sp->oldbuf);
	      sp->flags = _POP3F_DONE;
	    }
	}
    }
}

static int
mu_pop3_filter_create (mu_stream_t *pstream, mu_stream_t stream)
{
  int rc;

  rc = mu_filter_create (pstream, stream, "CRLFDOT", MU_FILTER_DECODE,
			 MU_STREAM_READ);
  if (rc == 0)
    {
      struct mu_pop3_stream *sp = (struct mu_pop3_stream *) stream;
      mu_stream_t str = *pstream;
      
      str->event_cb = _pop3_event_cb;
      str->event_mask = _MU_STR_EVMASK(_MU_STR_EVENT_SETFLAG);

      sp->oldbuf.type = MU_TRANSPORT_OUTPUT;
      if (mu_stream_ioctl (sp->pop3->carrier, MU_IOCTL_TRANSPORT_BUFFER,
                           MU_IOCTL_OP_GET,
			   &sp->oldbuf) == 0)
	{
	  struct mu_buffer_query newbuf;

	  sp->flags |= _POP3F_CHBUF;
	  newbuf.type = MU_TRANSPORT_OUTPUT;
	  newbuf.buftype = mu_buffer_full;
	  newbuf.bufsize = 64*1024;
	  mu_stream_ioctl (sp->pop3->carrier, MU_IOCTL_TRANSPORT_BUFFER,
                           MU_IOCTL_OP_SET, 
			   &newbuf);
	}
    }
  return rc;
}


static int
_mu_pop3_read (struct _mu_stream *str, char *buf, size_t bufsize,
	       size_t *pnread)
{
  struct mu_pop3_stream *sp = (struct mu_pop3_stream *)str;
  mu_pop3_t pop3 = sp->pop3;
  size_t nread;
  int status = 0;

  if (sp->flags & _POP3F_DONE)
    nread = 0;
  else
    {
      status = mu_stream_read (pop3->carrier, buf, bufsize, &nread);
      if (status == 0 && nread == 0)
	{
	  pop3->state = MU_POP3_NO_STATE;
	  sp->flags |= _POP3F_DONE;
	}
    }
  *pnread = nread;
  return status;
}

static int
_mu_pop3_readdelim (struct _mu_stream *str, char *buf, size_t bufsize,
		    int delim, size_t *pnread)
{
  struct mu_pop3_stream *sp = (struct mu_pop3_stream *)str;
  mu_pop3_t pop3 = sp->pop3;
  size_t nread;
  int status = 0;
  
  if (sp->flags & _POP3F_DONE)
    nread = 0;
  else
    {
      status = mu_stream_readdelim (pop3->carrier, buf, bufsize, delim,
				    &nread);
      if (status == 0 && nread == 0)
	{
	  pop3->state = MU_POP3_NO_STATE;
	  sp->flags |= _POP3F_DONE;
	}
    }
  *pnread = nread;
  return status;
}

static int
_mu_pop3_flush (struct _mu_stream *str)
{
  struct mu_pop3_stream *sp = (struct mu_pop3_stream *)str;
  mu_pop3_t pop3 = sp->pop3;
  return mu_stream_flush (pop3->carrier);
}

static int
_mu_pop3_wait (struct _mu_stream *str, int *pflags, struct timeval *tvp)
{
  struct mu_pop3_stream *sp = (struct mu_pop3_stream *)str;
  mu_pop3_t pop3 = sp->pop3;
  return mu_stream_wait (pop3->carrier, pflags, tvp);
}

int
mu_pop3_stream_create (mu_pop3_t pop3, mu_stream_t *pstream)
{
  struct mu_pop3_stream *sp;
  mu_stream_t str;
  int status;
  
  sp = (struct mu_pop3_stream *) _mu_stream_create (sizeof (*sp),
						    MU_STREAM_READ |
						    _MU_STR_OPEN);
  if (!sp)
    return ENOMEM;
  sp->stream.read = _mu_pop3_read; 
  sp->stream.readdelim = _mu_pop3_readdelim; 
  sp->stream.flush = _mu_pop3_flush;
  sp->stream.wait = _mu_pop3_wait;
  
  sp->pop3 = pop3;
  sp->flags = 0;
  str = (mu_stream_t) sp;
  mu_stream_set_buffer (str, mu_buffer_line, 0);

  status = mu_pop3_filter_create (pstream, str);
  mu_stream_unref (str);
  return status;
}
