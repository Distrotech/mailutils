/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2004 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <mailutils/stream.h>
#include <mailutils/filter.h>
#include <mailutils/errno.h>
#include <mailutils/nls.h>

#ifdef HAVE_ICONV_H
# include <iconv.h>
#endif

#ifndef ICONV_CONST
# define ICONV_CONST
#endif

#ifndef HAVE_ICONV
# undef iconv_open
# define iconv_open(tocode, fromcode) ((iconv_t) -1)

# undef iconv
# define iconv(cd, inbuf, inbytesleft, outbuf, outbytesleft) (errno = EILSEQ, (size_t) -1)

# undef iconv_close
# define iconv_close(cd) 0

#endif 

enum _icvt_state
  {
    state_closed,
    state_open,
    state_literal,
    state_octal,
    state_iconv_error,
    state_transport_error
  };

struct icvt_stream
{
  stream_t stream;   /* I/O stream */
  iconv_t cd;        /* Conversion descriptor */
  char *buf;         /* Conversion buffer */
  size_t bufsize;    /* Size of buf */
  size_t bufpos;     /* Current position in buf */
  enum _icvt_state state;
  int ec;            /* Error code */
  char ICONV_CONST *errp;  /* Offending input if state == state_iconv_error */
  char errbuf[128];  /* Error message buffer */
};

static int
_icvt_open (stream_t stream)
{
  struct icvt_stream *s = stream_get_owner (stream);
  if (s->cd == (iconv_t) -1)
    return EINVAL;
  s->state = state_open;
  return 0;
}

static int
_icvt_close (stream_t stream)
{
  struct icvt_stream *s = stream_get_owner (stream);
  if (s->state != state_closed)
    {
      int flags;
      stream_get_flags (stream, &flags);
      if (!(flags & MU_STREAM_NO_CLOSE))
	stream_close (s->stream);
      iconv_close (s->cd);
      s->cd = (iconv_t) -1;
      s->state = state_closed;
    }
  return 0;
}

static void
_icvt_destroy (stream_t stream)
{
  struct icvt_stream *s = stream_get_owner (stream);
  int flags;

  if (s->state != state_closed)
    _icvt_close (stream);
  stream_get_flags (stream, &flags);
  if (!(flags & MU_STREAM_NO_CLOSE))
    stream_destroy (&s->stream, stream_get_owner (s->stream));
  free (s->buf);
  s->buf = NULL;
  if (s->cd != (iconv_t) -1)
    iconv_close (s->cd);
}

static int _icvt_read (stream_t stream, char *optr, size_t osize,
		       off_t offset, size_t *pnbytes);

static int
internal_icvt_read (stream_t stream, char *optr, size_t osize, size_t *pnbytes)
{
  struct icvt_stream *s = stream_get_owner (stream);
  size_t nbytes = 0;
  size_t cvtbytes = 0;
  int rc, status;

  if (s->bufpos == 0)
    {
      status = stream_sequential_read (s->stream, s->buf, s->bufsize, &nbytes);
      if (status)
	{
	  s->state = state_transport_error;
	  s->ec = rc;
	  return MU_ERR_FAILURE;
	}
      else if (nbytes == 0)
	{
	  if (*pnbytes)
	    *pnbytes = 0;
	  return 0;
	}
    }
  
  do
    {
      char ICONV_CONST *ib = s->buf;
      size_t inlen = s->bufpos + nbytes;
      char *ob = optr + cvtbytes;
      size_t olen = osize - cvtbytes;
      
      rc = iconv (s->cd, &ib, &inlen, &ob, &olen);
      cvtbytes += ib - s->buf;
      if (rc == -1)
	{
	  if (errno == E2BIG)
	    {
	      if (cvtbytes)
		{
		  memmove (s->buf, ib, inlen);
		  s->bufpos = inlen;
		  break;
		}
	      else
		{
		  s->ec = MU_ERR_BUFSPACE;
		  return MU_ERR_BUFSPACE;
		}
	    }
	  else if (errno == EILSEQ)
	    {
	      int flags = 0; 
	      stream_get_flags (stream, &flags);
	      if (flags & MU_STREAM_STRICT)
		{
		  s->state = state_iconv_error;
		  s->ec = errno;
		  s->errp = ib;
		  if (cvtbytes)
		    break;
		  else
		    return MU_ERR_FAILURE;
		}
	      else
		{
		  s->state = state_octal;
		  memmove (s->buf, ib, inlen);
		  s->bufpos = inlen;		  
		  if (cvtbytes == 0)
		    return _icvt_read (stream, optr, osize, 0, pnbytes);
		  break;
		}
	    }
	  else if (errno == EINVAL)
	    {
	      if (inlen == s->bufsize)
		{
		  /* Try to reallocate temp buffer */
		  char *p = realloc (s->buf, s->bufsize + 128);
		  if (!p)
		    {
		      /* Rearrange the buffer anyway */
		      memmove (s->buf, ib, inlen);
		      s->bufpos = inlen;
		      return ENOMEM;
		    }
		  s->bufsize += 128;
		}
	      memmove (s->buf, ib, inlen);
	      s->bufpos = inlen;
	      continue;
	    }
	  else
	    {
	      s->ec = errno;
	      s->state = state_iconv_error;
	      return MU_ERR_FAILURE;
	    }
	}
    }
  while (cvtbytes < osize
	 && (status = stream_sequential_read (s->stream,
					      s->buf + s->bufpos,
					      s->bufsize - s->bufpos,
					      &nbytes)) == 0
	 && nbytes);

  if (status)
    {
      s->state = state_transport_error;
      s->ec = rc;
      if (!cvtbytes)
	return MU_ERR_FAILURE;
    }
      
  if (*pnbytes)
    *pnbytes = cvtbytes;
  return 0;
}

#define ISPRINT(c) ((c)>=' '&&(c)<127)

static int
octal_icvt_read (struct icvt_stream *s, char *optr, size_t osize, size_t *pnbytes)
{
  size_t i, j;
  int status;
  
  if (osize == 0)
    return MU_ERR_BUFSPACE;
  
  if (s->bufpos < osize)
    {
      size_t rdcount = osize;
      if (s->bufsize < rdcount)
	{
	  /* Try to reallocate temp buffer */
	  char *p = realloc (s->buf, rdcount);
	  if (p)
	    s->bufsize = rdcount;
	  else
	    rdcount = s->bufsize;
	}

      status = stream_sequential_read (s->stream, s->buf, rdcount - s->bufpos,
				       &rdcount);
      if (status)
	{
	  s->state = state_transport_error;
	  s->ec = status;
	  if (s->bufpos == 0)
	    return MU_ERR_FAILURE;
	}
      else
	s->bufpos += rdcount;
    }
  
  for (i = j = 0; i < osize && j < s->bufpos; i++)
    {
      if (ISPRINT (*(unsigned char*)(s->buf+i)))
	optr[j++] = s->buf[i];
      else if (j + 4 >= s->bufpos)
	break;
      else
	{
	  sprintf (optr + j, "\\%03o", *(unsigned char*)(s->buf+i));
	  j += 4;
	}
    }
  s->bufpos -= i;
  memmove (s->buf, s->buf + i, s->bufpos);
  if (pnbytes)
    *pnbytes = j;
  return 0;
}

static int
literal_icvt_read (struct icvt_stream *s, char *optr, size_t osize, size_t *pnbytes)
{
  int status;
  size_t nbytes;
  
  if (s->bufpos)
    {
      size_t sz = s->bufpos < osize ? s->bufpos : osize;
      memcpy (optr, s->buf, sz);
      s->bufpos -= sz;
      if (s->bufpos)
	memmove (s->buf, s->buf + sz, s->bufpos);
      if (*pnbytes)
	*pnbytes = sz;
      return 0;
    }

  status = stream_sequential_read (s->stream, optr, osize, &nbytes);
  if (status)
    {
      s->state = state_transport_error;
      s->ec = status;
      if (s->bufpos == 0)
	return MU_ERR_FAILURE;
    }
  if (*pnbytes)
    *pnbytes = nbytes;
  return 0;
}

static int
_icvt_read (stream_t stream, char *optr, size_t osize,
	    off_t offset ARG_UNUSED, size_t *pnbytes)
{
  struct icvt_stream *s = stream_get_owner (stream);

  switch (s->state)
    {
    case state_open:
      return internal_icvt_read (stream, optr, osize, pnbytes);
      
    case state_closed:
      return EINVAL;
      
    case state_literal:
      return literal_icvt_read (s, optr, osize, pnbytes);
      
    case state_octal:
      return octal_icvt_read (s, optr, osize, pnbytes);
	
    default:
      break;
    }
  return MU_ERR_FAILURE;
}

int
_icvt_strerror (stream_t stream, char **pstr)
{
  struct icvt_stream *s = stream_get_owner (stream);
  switch (s->state)
    {
    case state_transport_error:
      snprintf (s->errbuf, sizeof s->errbuf,
		_("Transport error: %s"), mu_strerror (s->ec));
      *pstr = s->errbuf;
      break;

    case state_iconv_error:
      switch (s->ec)
	{
	case EILSEQ:
	  snprintf (s->errbuf, sizeof s->errbuf,
		    _("Illegal multibyte sequence near %s"), s->errp);
	  break;

	default:
	  snprintf (s->errbuf, sizeof s->errbuf,
		    _("Iconv error: %s"), mu_strerror (s->ec));
	}
      *pstr = s->errbuf;
      break;

    case state_closed:
      *pstr = _("Stream is closed"); 
      break;
      
    default:
      *pstr = (char*) mu_strerror (s->ec);
    }
  
  return 0;
}

int
_icvt_get_transport2 (stream_t stream,
		     mu_transport_t *pt, mu_transport_t *pt2)
{
  struct icvt_stream *s = stream_get_owner (stream);
  if (pt)
    *pt = s->stream;
  if (*pt2)
    *pt2 = NULL;
  return 0;
}

int
_icvt_wait (stream_t stream, int *pflags, struct timeval *tvp)
{
  struct icvt_stream *s = stream_get_owner (stream);
  return stream_wait (s->stream, pflags, tvp);
}

int
filter_iconv_create (stream_t *s, stream_t transport,
		     char *fromcode, char *tocode, int flags)
{
  struct icvt_stream *iptr;
  iconv_t cd;
  int rc;
  
  cd = iconv_open (tocode, fromcode);
  if (cd == (iconv_t) -1)
    return MU_ERR_FAILURE;
  
  iptr = malloc (sizeof *iptr);
  if (!iptr)
    return ENOMEM;
  iptr->stream = transport;
  iptr->cd = cd;
  iptr->state = state_closed;
  iptr->bufsize = 128;
  iptr->buf = malloc (iptr->bufsize);
  if (!iptr->buf)
    {
      free (iptr);
      return ENOMEM;
    }
  iptr->bufpos = 0;
  
  rc = stream_create (s, flags, iptr);
  if (rc)
    {
      free (iptr);
      return rc;
    }
  stream_set_open (*s, _icvt_open, iptr);
  stream_set_close (*s, _icvt_close, iptr);
  stream_set_read (*s, _icvt_read, iptr);
  stream_set_destroy (*s, _icvt_destroy, iptr);
  stream_set_strerror (*s, _icvt_strerror, iptr);
  stream_set_get_transport2 (*s, _icvt_get_transport2, iptr);
  stream_set_wait (*s, _icvt_wait, iptr);
  return 0;
}
