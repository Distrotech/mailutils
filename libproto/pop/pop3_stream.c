/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003, 2007, 2010 Free Software Foundation, Inc.

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

#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <mailutils/filter.h>
#include <mailutils/stream.h>
#include <mailutils/sys/stream.h>
#include <mailutils/sys/pop3.h>

/* Implementation of the stream for TOP and RETR.  */
struct mu_pop3_stream
{
  struct _mu_stream stream;
  mu_pop3_t pop3;
  int done;
};

enum pop3_decode_state
  {
    pds_init,  /* initial state */
    pds_char,  /* Any character excepting [\r\n.] */
    pds_cr,    /* prev. char was \r */
    pds_crlf,  /* 2 prev. char were \r\n */
    pds_dot,   /* 3 prev. chars were \r\n. */
    pds_dotcr, /* 4 prev. chars were \r\n.\r */
    pds_end    /* final state, a \r\n.\r\n seen. */
  };

static int
newstate (int state, int c)
{
  switch (state)
    {
    case pds_init:
      switch (c)
	{
	case '\r':
	  return pds_cr;
	case '.':
	  return pds_dot;
	}
      break;
      
    case pds_char:
      switch (c)
	{
	case '\r':
	  return pds_cr;
	}
      break;
      
    case pds_cr:
      switch (c)
	{
	case '\r':
	  return pds_cr;
	case '\n':
	  return pds_crlf;
	}
      break;
      
    case pds_crlf:
      switch (c)
	{
	case '\r':
	  return pds_cr;
	case '.':
	  return pds_dot;
	}
      
    case pds_dot:
      switch (c)
	{
	case '\r':
	  return pds_dotcr;
	}
      break;

    case pds_dotcr:
      switch (c)
	{
	case '\n':
	  return pds_end;
	}
    }
  return pds_char;
}

/* Move min(isize,osize) bytes from iptr to optr, replacing each \r\n
   with \n. */
static enum mu_filter_result
_pop3_decoder (void *xd,
	       enum mu_filter_command cmd,
	       struct mu_filter_io *iobuf)
{
  int *pstate = xd;
  size_t i, j;
  const unsigned char *iptr;
  size_t isize;
  char *optr;
  size_t osize;

  switch (cmd)
    {
    case mu_filter_init:
      *pstate = pds_init;
      return mu_filter_ok;
      
    case mu_filter_done:
      return mu_filter_ok;
      
    default:
      break;
    }
  
  iptr = (const unsigned char *) iobuf->input;
  isize = iobuf->isize;
  optr = iobuf->output;
  osize = iobuf->osize;

  for (i = j = 0; *pstate != pds_end && i < isize && j < osize; i++)
    {
      unsigned char c = *iptr++;

      if (c == '\r')
	{
	  if (i + 1 == isize)
	    break;
	  *pstate = newstate (*pstate, c);
	  if (*iptr == '\n')
	    continue;
	}
      else if (c == '.' && (*pstate == pds_init || *pstate == pds_crlf))
	{
	  /* Make sure we have two more characters in the buffer */
	  if (i + 2 == isize)
	    break;
	  *pstate = newstate (*pstate, c);
	  if (*iptr != '\r')
	    continue;
	}
      else
	*pstate = newstate (*pstate, c);
      optr[j++] = c;
    }
  
  if (*pstate == pds_end)
    {
      j -= 2; /* remove the trailing .\n */
      iobuf->eof = 1;
    }
  iobuf->isize = i;
  iobuf->osize = j;
  return mu_filter_ok;
}

static void
_pop3_event_cb (mu_stream_t str, int ev, int flags)
{
  if (ev == _MU_STR_EVENT_SET)
    {
      mu_transport_t trans[2];

      if (mu_stream_ioctl (str, MU_IOCTL_GET_TRANSPORT, trans) == 0)
	{
	  struct mu_pop3_stream *sp = (struct mu_pop3_stream *) trans[0];
	  _mu_pop3_xscript_level (sp->pop3, XSCRIPT_NORMAL);
	  sp->pop3->state = MU_POP3_NO_STATE;
	}
    }
}

static int
mu_pop3_filter_create (mu_stream_t *pstream, mu_stream_t stream)
{
  int rc;
  int *state = malloc (sizeof (*state));
  if (!state)
    return ENOMEM;
  rc = mu_filter_stream_create (pstream, stream,
				MU_FILTER_DECODE,
				_pop3_decoder, state,
				MU_STREAM_READ);
  if (rc == 0)
    {
      mu_stream_t str = *pstream;
      str->event_cb = _pop3_event_cb;
      str->event_mask = _MU_STR_EOF;
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

  if (sp->done)
    nread = 0;
  else
    {
      status = mu_stream_read (pop3->carrier, buf, bufsize, &nread);
      if (status == 0 && nread == 0)
	{
	  pop3->state = MU_POP3_NO_STATE;
	  sp->done = 1;
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
  
  if (sp->done)
    nread = 0;
  else
    {
      status = mu_stream_readdelim (pop3->carrier, buf, bufsize, delim,
				    &nread);
      if (status == 0 && nread == 0)
	{
	  pop3->state = MU_POP3_NO_STATE;
	  sp->done = 1;
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
						    MU_STREAM_READ);
  if (!sp)
    return ENOMEM;
  sp->stream.read = _mu_pop3_read; 
  sp->stream.readdelim = _mu_pop3_readdelim; 
  sp->stream.flush = _mu_pop3_flush;
  sp->stream.wait = _mu_pop3_wait;
  
  sp->pop3 = pop3;
  sp->done = 0;
  str = (mu_stream_t) sp;
  mu_stream_set_buffer (str, mu_buffer_line, 1024);

  status = mu_pop3_filter_create (pstream, str);
  mu_stream_unref (str);
  return status;
}
