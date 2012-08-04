/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2009-2012 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>. */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#ifndef SIZE_MAX
# define SIZE_MAX (~((size_t)0))
#endif

#include <mailutils/types.h>
#include <mailutils/alloc.h>
#include <mailutils/error.h>
#include <mailutils/errno.h>
#include <mailutils/nls.h>
#include <mailutils/stream.h>
#include <mailutils/sys/stream.h>

size_t mu_stream_default_buffer_size = MU_STREAM_DEFBUFSIZ;

#define _MU_STR_FLUSH_ALL  0x01
#define _MU_STR_FLUSH_KEEP 0x02

#define _stream_event(stream, code, n, p)			\
  do								\
    {								\
      if ((stream)->event_cb &&					\
          ((stream)->event_mask & _MU_STR_EVMASK(code)))	\
        (stream)->event_cb (stream, code, n, p);		\
    }								\
  while (0)

#define _bootstrap_event(stream)					    \
  do									    \
    {									    \
      if ((stream)->event_cb &&						    \
          ((stream)->event_mask & _MU_STR_EVMASK(_MU_STR_EVENT_BOOTSTRAP))) \
	{								    \
	  (stream)->event_cb (stream, _MU_STR_EVENT_BOOTSTRAP, 0, NULL);    \
	  (stream)->event_mask &= ~_MU_STR_EVMASK(_MU_STR_EVENT_BOOTSTRAP); \
	}								    \
    }									    \
  while (0)
  

#define _stream_stat_incr(s, k, n) \
  (((s)->statmask & MU_STREAM_STAT_MASK(k)) ? ((s)->statbuf[k] += n) : 0)

#define _stream_read(str, buf, size, rdbytes) \
  (_stream_stat_incr ((str), MU_STREAM_STAT_READS, 1),  \
   (str)->read (str, buf, size, rdbytes))
#define _stream_write(str, buf, size, wrbytes) \
  (_stream_stat_incr ((str), MU_STREAM_STAT_WRITES, 1),  \
   (str)->write (str, buf, size, wrbytes))
#define _stream_seek(str, pos, poff)			\
  (_stream_stat_incr ((str), MU_STREAM_STAT_SEEKS, 1),		\
   (str)->seek (str, pos, poff))

static int _stream_read_unbuffered (mu_stream_t stream, void *buf, size_t size,
				    int full_read, size_t *pnread);
static int _stream_write_unbuffered (mu_stream_t stream,
				     const void *buf, size_t size,
				     int full_write, size_t *pnwritten);

static void
_stream_setflag (struct _mu_stream *stream, int flag)
{
  _stream_event (stream, _MU_STR_EVENT_SETFLAG, flag, NULL);
  stream->flags |= flag;
}

static void
_stream_clrflag (struct _mu_stream *stream, int flag)
{
  _stream_event (stream, _MU_STR_EVENT_CLRFLAG, flag, NULL);
  stream->flags &= ~flag;
}

int
mu_stream_seterr (struct _mu_stream *stream, int code, int perm)
{
  stream->last_err = code;
  switch (code)
    {
    case 0:
    case EAGAIN:
    case EINPROGRESS:
      break;

    default:
      if (perm)
	_stream_setflag (stream, _MU_STR_ERR);
    }
  return code;
}

void
_mu_stream_cleareof (mu_stream_t str)
{
  _stream_clrflag (str, _MU_STR_EOF);
}

void
_mu_stream_seteof (mu_stream_t str)
{
  _stream_setflag (str, _MU_STR_EOF);
}

#define _stream_buffer_freespace(s) \
  ((s)->bufsize - (s)->level)
#define _stream_buffer_is_full(s) (_stream_buffer_freespace(s) == 0)

#define _stream_curp(s) ((s)->buffer + (s)->pos)

static int
_stream_fill_buffer (struct _mu_stream *stream)
{
  size_t n;
  size_t rdn;
  int rc = 0;
  char c;

  switch (stream->buftype)
    {
    case mu_buffer_none:
      return 0;
	
    case mu_buffer_full:
      rc = _stream_read_unbuffered (stream,
				    stream->buffer, stream->bufsize,
				    0,
				    &stream->level);
      break;
	
    case mu_buffer_line:
      for (n = 0;
	   n < stream->bufsize
	     && (rc = _stream_read_unbuffered (stream,
					       &c, 1,
					       0, &rdn)) == 0;)
	{
	  if (rdn == 0)
	    {
	      _stream_setflag (stream, _MU_STR_EOF);
	      break;
	    }
	  stream->buffer[n++] = c;
	  if (c == '\n')
	    break;
	}
      stream->level = n;
      break;
    }
  if (rc == 0)
    {
      stream->pos = 0;
      _stream_event (stream, _MU_STR_EVENT_FILLBUF,
		     stream->level, _stream_curp (stream));
    }
  return rc;
}

static int
_stream_buffer_full_p (struct _mu_stream *stream)
{
    switch (stream->buftype)
      {
      case mu_buffer_none:
	break;
	
      case mu_buffer_line:
	return _stream_buffer_is_full (stream)
	       || memchr (stream->buffer, '\n', stream->level) != NULL;

      case mu_buffer_full:
	return _stream_buffer_is_full (stream);
      }
    return 0;
}

static int
_stream_flush_buffer (struct _mu_stream *stream, int flags)
{
  int rc;
  char *start, *end;
  size_t wrsize;

  if (stream->flags & _MU_STR_DIRTY)
    {
      if ((stream->flags & MU_STREAM_SEEK) && stream->seek)
	{
	  mu_off_t off;
	  rc = _stream_seek (stream, stream->offset, &off);
	  if (rc)
	    return rc;
	}

      switch (stream->buftype)
	{
	case mu_buffer_none:
	    abort(); /* should not happen */
	    
	case mu_buffer_full:
	  if ((rc = _stream_write_unbuffered (stream, stream->buffer,
					      stream->level,
					      1, NULL)))
	    return rc;
	  _stream_event (stream, _MU_STR_EVENT_FLUSHBUF,
			 stream->level, stream->buffer);
	  break;
	    
	case mu_buffer_line:
	  if (stream->level == 0)
	    break;
	  wrsize = stream->level;
	  for (start = stream->buffer, end = memchr (start, '\n', wrsize);
	       end;
	       end = memchr (start, '\n', wrsize))
	    {
	      size_t size = end - start + 1;
	      rc = _stream_write_unbuffered (stream, start, size,
					     1, NULL);
	      if (rc)
		return rc;
	      _stream_event (stream, _MU_STR_EVENT_FLUSHBUF,
			     size, start);
	      start += size;
	      wrsize -= size;
	      if (wrsize == 0)
		break;
	    }
	  if (((flags & _MU_STR_FLUSH_ALL) && wrsize) ||
	      wrsize == stream->level)
	    {
	      rc = _stream_write_unbuffered (stream,
					     stream->buffer,
					     wrsize,
					     1, NULL);
	      if (rc)
		return rc;
	      _stream_event (stream, _MU_STR_EVENT_FLUSHBUF,
			     wrsize, stream->buffer);
	      wrsize = 0;
	    }
	  if (!(flags & _MU_STR_FLUSH_KEEP))
	    {
	      if (wrsize)
		memmove (stream->buffer, start, wrsize);
	      else
		_stream_clrflag (stream, _MU_STR_DIRTY);
	      stream->offset += stream->level - wrsize;
	      stream->level = stream->pos = wrsize;
	      return 0;
	    }
	}
      _stream_clrflag (stream, _MU_STR_DIRTY);
    }

  if (!(flags & _MU_STR_FLUSH_KEEP))
    {
      stream->offset += stream->level;
      stream->pos = stream->level = 0;
    }
  
  return 0;
}


mu_stream_t
_mu_stream_create (size_t size, int flags)
{
  struct _mu_stream *str;
  if (size < sizeof (str))
    abort ();
  str = mu_zalloc (size);
  str->flags = flags & ~(_MU_STR_INTERN_MASK & ~_MU_STR_OPEN);
  mu_stream_ref (str);
  return str;
}

void
mu_stream_destroy (mu_stream_t *pstream)
{
  if (pstream)
    {
      mu_stream_t str = *pstream;
      if (str && (str->ref_count == 0 || --str->ref_count == 0))
	{
	  mu_stream_close (str);
	  if (str->done)
	    str->done (str);
	  if (str->destroy)
	    str->destroy (str);
	  else
	    free (str);
	  *pstream = NULL;
	}
    }
}

void
mu_stream_get_flags (mu_stream_t str, int *pflags)
{
  *pflags = str->flags & ~_MU_STR_INTERN_MASK;
}
  
void
mu_stream_ref (mu_stream_t stream)
{
  stream->ref_count++;
}

void
mu_stream_unref (mu_stream_t stream)
{
  mu_stream_destroy (&stream);
}

static void
_stream_init (mu_stream_t stream)
{
  if (stream->statmask)
    memset (stream->statbuf, 0,
	    _MU_STREAM_STAT_MAX * sizeof (stream->statbuf[0]));
  stream->flags &= ~_MU_STR_INTERN_MASK;
  _stream_setflag (stream, _MU_STR_OPEN);
  stream->offset = 0;
  stream->level = stream->pos = 0;
  stream->last_err = 0;
}  

int
mu_stream_open (mu_stream_t stream)
{
  int rc;

  if (stream->flags & _MU_STR_OPEN)
    return MU_ERR_OPEN;
  _bootstrap_event (stream);
  if (stream->open)
    {
      if ((rc = stream->open (stream)))
	return mu_stream_seterr (stream, rc, 1);
    }
  _stream_init (stream);
  if ((stream->flags & (MU_STREAM_APPEND|MU_STREAM_SEEK)) ==
      (MU_STREAM_APPEND|MU_STREAM_SEEK) &&
      (rc = mu_stream_seek (stream, 0, MU_SEEK_END, NULL)))
    return mu_stream_seterr (stream, rc, 1);
  return 0;
}

const char *
mu_stream_strerror (mu_stream_t stream, int rc)
{
  const char *str;

  if (stream->error_string)
    str = stream->error_string (stream, rc);
  else
    str = mu_strerror (rc);
  return str;
}

int
mu_stream_err (mu_stream_t stream)
{
  return stream->flags & _MU_STR_ERR;
}

int
mu_stream_last_error (mu_stream_t stream)
{
  return stream->last_err;
}

void
mu_stream_clearerr (mu_stream_t stream)
{
  stream->last_err = 0;
  _stream_clrflag (stream, _MU_STR_ERR);
}

int
mu_stream_eof (mu_stream_t stream)
{
  return (stream->flags & _MU_STR_EOF) && (stream->pos == stream->level);
}

int
mu_stream_is_open (mu_stream_t stream)
{
  return stream->flags & _MU_STR_OPEN;
}

int
mu_stream_seek (mu_stream_t stream, mu_off_t offset, int whence,
		mu_off_t *pres)
{    
  int rc;
  mu_off_t size;
  
  _bootstrap_event (stream);
  if (!(stream->flags & _MU_STR_OPEN))
    {
      if (stream->open)
	return MU_ERR_NOT_OPEN;
      _stream_init (stream);
    }
  
  if (!stream->seek)
    return mu_stream_seterr (stream, ENOSYS, 0);

  if (!(stream->flags & MU_STREAM_SEEK))
    return mu_stream_seterr (stream, EACCES, 1);

  switch (whence)
    {
    case MU_SEEK_SET:
      break;

    case MU_SEEK_CUR:
      if (offset == 0)
	{
	  *pres = stream->offset + stream->pos;
	  return 0;
	}
      offset += stream->offset + stream->pos;
      break;

    case MU_SEEK_END:
      rc = mu_stream_size (stream, &size);
      if (rc)
	return rc;
      offset += size;
      break;

    default:
      return mu_stream_seterr (stream, EINVAL, 1);
    }

  if (!(stream->buftype == mu_buffer_none ?
	(offset == stream->offset)
	: (stream->offset <= offset &&
	   offset < stream->offset + stream->level)))
    {
      if ((rc = _stream_flush_buffer (stream, _MU_STR_FLUSH_ALL)))
	return rc;
      if (stream->offset != offset)
	{
	  rc = _stream_seek (stream, offset, &stream->offset);
	  if (rc == ESPIPE)
	    return rc;
	  if (rc)
	    return mu_stream_seterr (stream, rc, 1);
	}
      _mu_stream_cleareof (stream);
    }
  else if (stream->buftype != mu_buffer_none)
    stream->pos = offset - stream->offset;

  _mu_stream_cleareof (stream);
  
  if (pres)
    *pres = stream->offset + stream->pos;
  return 0;
}

/* Skip COUNT bytes from the current position in stream by reading from
   it.  Return new offset in PRES.

   Return 0 on success, EACCES if STREAM was not opened for reading.
   Another non-zero exit codes are propagated from the underlying
   input operations.
   
   This function is designed to help implement seek method in otherwise
   unseekable streams (such as filters).  Do not use it unless you absolutely
   have to.  Using it on an unbuffered stream is a terrible waste of CPU. */
static int
_stream_skip_input_bytes (mu_stream_t stream, mu_off_t count, mu_off_t *pres)
{
  mu_off_t pos;
  int rc = 0;

  if (!(stream->flags & MU_STREAM_READ))
    return mu_stream_seterr (stream, EACCES, 1);

  if (count)
    {
      if (stream->buftype == mu_buffer_none)
	{
	  for (pos = 0; pos < count; pos++)
	    {
	      char c;
	      size_t nrd;
	      rc = mu_stream_read (stream, &c, 1, &nrd);
	      if (nrd == 0)
		rc = ESPIPE;
	      if (rc)
		break;
	    }
	}
      else
	{
	  for (pos = 0;;)
	    {
	      if (stream->pos == stream->level)
		{
		  if ((rc = _stream_flush_buffer (stream, _MU_STR_FLUSH_ALL)))
		    return rc;
		  rc = _stream_fill_buffer (stream);
		  if (rc)
		    break;
		  if (stream->level == 0)
		    {
		      rc = ESPIPE;
		      break;
		    }
		}
	      if (pos <= count && count < pos + stream->level)
		{
		  stream->pos = count - pos;
		  rc = 0;
		  break;
		}
	      pos += stream->level;
	    }
	}
    }
  
  if (pres)
    *pres = stream->offset + stream->pos;
  return rc;
}

/* A wrapper for the above function.  It is normally called from a
   seek method implementation, so it makes sure the MU_STREAM_SEEK
   is cleared while in _stream_skip_input_bytes, to avoid infitite
   recursion that may be triggered by _stream_flush_buffer invoking
   stream->seek. */
int
mu_stream_skip_input_bytes (mu_stream_t stream, mu_off_t count, mu_off_t *pres)
{
  int rc;
  int seek_flag = stream->flags & MU_STREAM_SEEK;
  stream->flags &= ~MU_STREAM_SEEK;
  rc = _stream_skip_input_bytes (stream, count, pres);
  stream->flags |= seek_flag;
  return rc;
}

int
mu_stream_set_buffer (mu_stream_t stream, enum mu_buffer_type type,
		      size_t size)
{
  _bootstrap_event (stream);
  
  if (size == 0)
    size = mu_stream_default_buffer_size;

  if (stream->setbuf_hook)
    {
      int rc = stream->setbuf_hook (stream, type, size);
      if (rc)
	return rc;
    }
  
  if (stream->buffer)
    {
      mu_stream_flush (stream);
      free (stream->buffer);
    }

  stream->buftype = type;
  if (type == mu_buffer_none)
    {
      stream->buffer = NULL;
      return 0;
    }

  stream->buffer = mu_alloc (size);
  if (stream->buffer == NULL)
    {
      stream->buftype = mu_buffer_none;
      return mu_stream_seterr (stream, ENOMEM, 1);
    }
  stream->bufsize = size;
  stream->pos = 0;
  stream->level = 0;
    
  return 0;
}

int
mu_stream_get_buffer (mu_stream_t stream, struct mu_buffer_query *qry)
{
  qry->buftype = stream->buftype;
  qry->bufsize = stream->bufsize;
  return 0;
}

static int
_stream_read_unbuffered (mu_stream_t stream, void *buf, size_t size,
			 int full_read, size_t *pnread)
{
  int rc;
  size_t nread;
    
  if (!stream->read) 
    return mu_stream_seterr (stream, ENOSYS, 0);

  if (!(stream->flags & MU_STREAM_READ)) 
    return mu_stream_seterr (stream, EACCES, 1);
    
  if (stream->flags & _MU_STR_ERR)
    return stream->last_err;
    
  if (mu_stream_eof (stream) || size == 0)
    {
      if (pnread)
	*pnread = 0;
      return 0;
    }
    
  if (full_read)
    {
      size_t rdbytes;

      nread = 0;
      while (size > 0
	     && (rc = _stream_read (stream, buf, size, &rdbytes)) == 0)
	{
	  if (rdbytes == 0)
	    {
	      _stream_setflag (stream, _MU_STR_EOF);
	      break;
	    }
	  buf += rdbytes;
	  nread += rdbytes;
	  size -= rdbytes;
	  _stream_stat_incr (stream, MU_STREAM_STAT_IN, rdbytes);
	}
      if (size && rc)
	rc = mu_stream_seterr (stream, rc, 0);
    }
  else
    {
      rc = _stream_read (stream, buf, size, &nread);
      if (rc == 0)
	{
	  if (nread == 0)
	    _stream_setflag (stream, _MU_STR_EOF);
	  _stream_stat_incr (stream, MU_STREAM_STAT_IN, nread);
	}
      mu_stream_seterr (stream, rc, rc != 0);
    }
  if (pnread)
    *pnread = nread;
  
  return rc;
}

static int
_stream_write_unbuffered (mu_stream_t stream,
			  const void *buf, size_t size,
			  int full_write, size_t *pnwritten)
{
  int rc;
  size_t nwritten;
  
  if (!stream->write) 
    return mu_stream_seterr (stream, ENOSYS, 0);

  if (!(stream->flags & (MU_STREAM_WRITE|MU_STREAM_APPEND))) 
    return mu_stream_seterr (stream, EACCES, 1);

  if (stream->flags & _MU_STR_ERR)
    return stream->last_err;

  if (size == 0)
    {
      if (pnwritten)
	*pnwritten = 0;
      return 0;
    }
    
  if (full_write)
    {
      size_t wrbytes;
      const char *bufp = buf;

      nwritten = 0;
      while (size > 0
	     && (rc = _stream_write (stream, bufp, size, &wrbytes))
	     == 0)
	{
	  if (wrbytes == 0)
	    {
	      rc = EIO;
	      break;
	    }
	  bufp += wrbytes;
	  nwritten += wrbytes;
	  size -= wrbytes;
	  _stream_stat_incr (stream, MU_STREAM_STAT_OUT, wrbytes);
	}
    }
  else
    {
      rc = _stream_write (stream, buf, size, &nwritten);
      if (rc == 0)
	_stream_stat_incr (stream, MU_STREAM_STAT_OUT, nwritten);
    }
  _stream_setflag (stream, _MU_STR_WRT);
  if (pnwritten)
    *pnwritten = nwritten;
  mu_stream_seterr (stream, rc, rc != 0);
  return rc;
}

int
mu_stream_read (mu_stream_t stream, void *buf, size_t size, size_t *pread)
{
  _bootstrap_event (stream);
  
  if (!(stream->flags & _MU_STR_OPEN))
    {
      if (stream->open)
	return MU_ERR_NOT_OPEN;
      _stream_init (stream);
    }
  
  if (stream->buftype == mu_buffer_none)
    {
      size_t nread = 0;
      int rc = _stream_read_unbuffered (stream, buf, size, !pread, &nread);
      stream->offset += nread;
      if (pread)
	*pread = nread;
      return rc;
    }
  else
    {
      char *bufp = buf;
      size_t nbytes = 0;
      int rc;
      
      if ((rc = _stream_flush_buffer (stream,
				      _MU_STR_FLUSH_ALL|_MU_STR_FLUSH_KEEP)))
	return rc;

      while (size)
	{
	  size_t n;
	  
	  if (stream->pos == stream->level)
	    {
	      if ((rc = _stream_flush_buffer (stream, _MU_STR_FLUSH_ALL)))
		{
		  if (nbytes)
		    break;
		  return rc;
		}
	      if ((rc = _stream_fill_buffer (stream)))
		{
		  if (nbytes)
		    break;
		  return rc;
		}
	      if (stream->level == 0)
		break;
	    }
	  
	  n = size;
	  if (n > stream->level - stream->pos)
	    n = stream->level - stream->pos;
	  memcpy (bufp, _stream_curp (stream), n);
	  stream->pos += n;
	  nbytes += n;
	  bufp += n;
	  size -= n;
	  if (stream->buftype == mu_buffer_line && bufp[-1] == '\n')
	    break;
	}
      
      if (pread)
	*pread = nbytes;
    }
  return 0;
}

int
_stream_scandelim (mu_stream_t stream, char *buf, size_t size, int delim,
		   size_t *pnread)
{
  int rc = 0;
  size_t nread = 0;
  
  size--;
  if (size == 0)
    return MU_ERR_BUFSPACE;
  while (size)
    {
      char *p, *q;
      size_t len;
      
      if (stream->pos == stream->level)
	{
	  if ((rc = _stream_flush_buffer (stream, _MU_STR_FLUSH_ALL)))
	    break;
	  if ((rc = _stream_fill_buffer (stream)) || stream->level == 0)
	    break;
	}

      q = _stream_curp (stream);
      len = stream->level - stream->pos;
      p = memchr (q, delim, len);
      if (p)
	len = p - q + 1;
      if (len > size)
	len = size;
      memcpy (buf, _stream_curp (stream), len);
      stream->pos += len;
      buf += len;
      size -= len;
      nread += len;
      if (p) /* Delimiter found */
	break;
    }
  *buf = 0;
  *pnread = nread;
  return rc;
}

static int
_stream_readdelim (mu_stream_t stream, char *buf, size_t size,
		   int delim, size_t *pread)
{
  int rc;
  char c;
  size_t n = 0, rdn;
    
  size--;
  if (size == 0)
    return MU_ERR_BUFSPACE;
  for (n = 0;
       n < size && (rc = mu_stream_read (stream, &c, 1, &rdn)) == 0 && rdn;)
    {
      *buf++ = c;
      n++;
      if (c == delim)
	break;
    }
  *buf = 0;
  if (pread)
    *pread = n;
  return rc;
}

int
mu_stream_readdelim (mu_stream_t stream, char *buf, size_t size,
		     int delim, size_t *pread)
{
  int rc;
  
  _bootstrap_event (stream);
  
  if (size == 0)
    return EINVAL;

  if (!(stream->flags & _MU_STR_OPEN))
    {
      if (stream->open)
	return MU_ERR_NOT_OPEN;
      _stream_init (stream);
    }

  if (stream->buftype == mu_buffer_none)
    {
      if (stream->readdelim)
	{
	  size_t nread;
	  rc = stream->readdelim (stream, buf, size, delim, &nread);
	  if (pread)
	    *pread = nread;
	  stream->offset += nread;
	}
      else
	rc = _stream_readdelim (stream, buf, size, delim, pread);
    }
  else
    {
      if ((rc = _stream_flush_buffer (stream,
				      _MU_STR_FLUSH_ALL|_MU_STR_FLUSH_KEEP)))
	return rc;
      rc = _stream_scandelim (stream, buf, size, delim, pread);
    }
  return rc;
}

int
mu_stream_readline (mu_stream_t stream, char *buf, size_t size, size_t *pread)
{
  return mu_stream_readdelim (stream, buf, size, '\n', pread);
}

int
mu_stream_getdelim (mu_stream_t stream, char **pbuf, size_t *psize,
		    int delim, size_t *pread)
{
  int rc;
  char *lineptr = *pbuf;
  size_t n = *psize;
  size_t cur_len = 0;

  _bootstrap_event (stream);
  
  if (!(stream->flags & _MU_STR_OPEN))
    {
      if (stream->open)
	return MU_ERR_NOT_OPEN;
      _stream_init (stream);
    }

  if ((rc = _stream_flush_buffer (stream,
				  _MU_STR_FLUSH_ALL|_MU_STR_FLUSH_KEEP)))
    return rc;
  
  if (lineptr == NULL || n == 0)
    {
      char *new_lineptr;
      n = 120;
      new_lineptr = mu_realloc (lineptr, n);
      if (new_lineptr == NULL) 
	return ENOMEM;
      lineptr = new_lineptr;
    }
    
  for (;;)
    {
      size_t rdn;

      /* Make enough space for len+1 (for final NUL) bytes.  */
      if (cur_len + 1 >= n)
	{
	  size_t needed_max =
	    SSIZE_MAX < SIZE_MAX ? (size_t) SSIZE_MAX + 1 : SIZE_MAX;
	  size_t needed = 2 * n + 1;   /* Be generous. */
	  char *new_lineptr;
	  
	  if (needed_max < needed)
	    needed = needed_max;
	  if (cur_len + 1 >= needed)
	    {
	      rc = EOVERFLOW;
	      break;
	    }
	    
	  new_lineptr = mu_realloc (lineptr, needed);
	  if (new_lineptr == NULL)
	    {
	      rc = ENOMEM;
	      break;
	    }
	    
	  lineptr = new_lineptr;
	  n = needed;
	}

      if (stream->readdelim)
	rc = stream->readdelim (stream, lineptr + cur_len, n - cur_len, delim,
				&rdn);
      else if (stream->buftype != mu_buffer_none)
	rc = _stream_scandelim (stream, lineptr + cur_len, n - cur_len, delim,
				&rdn);
      else
	rc = mu_stream_read (stream, lineptr + cur_len, 1, &rdn);

      if (rc || rdn == 0)
	break;
      cur_len += rdn;
      
      if (lineptr[cur_len - 1] == delim)
	break;
    }
  lineptr[cur_len] = '\0';
    
  *pbuf = lineptr;
  *psize = n;
  
  if (pread)
    *pread = cur_len;
  return rc;
}

int
mu_stream_getline (mu_stream_t stream, char **pbuf, size_t *psize,
		   size_t *pread)
{
  return mu_stream_getdelim (stream, pbuf, psize, '\n', pread);
}

int
mu_stream_write (mu_stream_t stream, const void *buf, size_t size,
		 size_t *pnwritten)
{
  int rc = 0;

  _bootstrap_event (stream);
  
  if (!(stream->flags & _MU_STR_OPEN))
    {
      if (stream->open)
	return MU_ERR_NOT_OPEN;
      _stream_init (stream);
    }

  if (stream->buftype == mu_buffer_none)
    {
      size_t nwritten;
      rc = _stream_write_unbuffered (stream, buf, size,
				     !pnwritten, &nwritten);
      stream->offset += nwritten;
      if (pnwritten)
	*pnwritten = nwritten;
    }
  else
    {
      size_t nbytes = 0;
      const char *bufp = buf;
	
      while (1)
	{
	  size_t n;
	  
	  if (_stream_buffer_full_p (stream)
	      && (rc = _stream_flush_buffer (stream, 0)))
	    break;

	  if (size == 0)
	    break;
	    
	  n = _stream_buffer_freespace (stream);
	  if (n > size)
	    n = size;
	  memcpy (_stream_curp (stream), bufp, n);
	  stream->pos += n;
	  if (stream->pos > stream->level)
	    stream->level = stream->pos;

	  nbytes += n;
	  bufp += n;
	  size -= n;
	  _stream_setflag (stream, _MU_STR_DIRTY);
	}
      if (pnwritten)
	*pnwritten = nbytes;
    }
  return rc;
}

int
mu_stream_writeline (mu_stream_t stream, const char *buf, size_t size)
{
  int rc;
  if ((rc = mu_stream_write (stream, buf, size, NULL)) == 0)
    rc = mu_stream_write (stream, "\r\n", 2, NULL);
  return rc;
}

int
mu_stream_flush (mu_stream_t stream)
{
  int rc;
  
  if (!stream)
    return EINVAL;
  _bootstrap_event (stream);
  if (!(stream->flags & _MU_STR_OPEN))
    {
      if (stream->open)
	return MU_ERR_NOT_OPEN;
      _stream_init (stream);
    }
  rc = _stream_flush_buffer (stream, _MU_STR_FLUSH_ALL);
  if (rc)
    return rc;
  if ((stream->flags & _MU_STR_WRT) && stream->flush)
    return stream->flush (stream);
  _stream_clrflag (stream, _MU_STR_WRT);
  return 0;
}

int
mu_stream_close (mu_stream_t stream)
{
  int rc = 0;
    
  if (!stream)
    return EINVAL;
  if (!(stream->flags & _MU_STR_OPEN))
    return MU_ERR_NOT_OPEN;
  
  mu_stream_flush (stream);
  /* Do close the stream only if it is not used by anyone else */
  if (stream->ref_count > 1)
    return 0;
  _stream_event (stream, _MU_STR_EVENT_CLOSE, 0, NULL);
  if (stream->close)
    rc = stream->close (stream);
  _stream_clrflag (stream, _MU_STR_OPEN);
  return rc;
}

int
mu_stream_size (mu_stream_t stream, mu_off_t *psize)
{
  int rc;
  mu_off_t size;

  _bootstrap_event (stream);
  if (!(stream->flags & _MU_STR_OPEN))
    {
      if (stream->open)
	return MU_ERR_NOT_OPEN;
      _stream_init (stream);
    }
  if (!stream->size)
    return mu_stream_seterr (stream, ENOSYS, 0);
  rc = stream->size (stream, &size);
  if (rc == 0)
    {
      if (stream->buftype != mu_buffer_none && stream->offset == size)
	size += stream->level;
      *psize = size;
    }
  return mu_stream_seterr (stream, rc, rc != 0);
}

int
mu_stream_ioctl (mu_stream_t stream, int family, int opcode, void *ptr)
{
  int rc;
  _bootstrap_event (stream);
  if ((rc = _stream_flush_buffer (stream, _MU_STR_FLUSH_ALL)))
    return rc;
  if (stream->ctl == NULL)
    return ENOSYS;
  return stream->ctl (stream, family, opcode, ptr);
}

int
mu_stream_wait (mu_stream_t stream, int *pflags, struct timeval *tvp)
{
  int flg = 0;

  if (stream == NULL)
    return EINVAL;
  _bootstrap_event (stream);
#if 0
  /* NOTE: Sometimes mu_stream_wait is called after a failed mu_stream_open.
     In particular, this is needed for a TCP stream opened with a
     MU_STREAM_NONBLOCK flag (see examples/http.c).  Until a better
     solution is found, this check is commented out. */
  if (!(stream->flags & _MU_STR_OPEN))
    {
      if (stream->open)
	return MU_ERR_NOT_OPEN;
      _stream_init (stream);
    }
#endif
  /* Take into account if we have any buffering.  */
  /* FIXME: How about MU_STREAM_READY_WR? */
  if ((*pflags) & MU_STREAM_READY_RD 
      && stream->buftype != mu_buffer_none
      && stream->pos < stream->level)
    {
      flg = MU_STREAM_READY_RD;
      *pflags &= ~MU_STREAM_READY_RD;
    }

  if (stream->wait)
    {
      int rc;

      if (flg && *pflags == 0)
	/* Don't call wait method if our modifications (see above) resulted
	   in empty *pflags. */
	rc = 0;
      else
        rc = stream->wait (stream, pflags, tvp);
      if (rc == 0)
	*pflags |= flg;
      return rc;
    }
  
  return ENOSYS;
}

int
mu_stream_truncate (mu_stream_t stream, mu_off_t size)
{
  _bootstrap_event (stream);
  
  if (!(stream->flags & _MU_STR_OPEN))
    {
      if (stream->open)
	return MU_ERR_NOT_OPEN;
      _stream_init (stream);
    }
  
  if (stream->truncate)
    {
      int rc;
      
      if ((rc = _stream_flush_buffer (stream, _MU_STR_FLUSH_ALL)))
	return rc;
      return stream->truncate (stream, size);
    }
  return ENOSYS;
}

int
mu_stream_shutdown (mu_stream_t stream, int how)
{
  int rc;

  _bootstrap_event (stream);
  
  if (!(stream->flags & _MU_STR_OPEN))
    {
      if (stream->open)
	return MU_ERR_NOT_OPEN;
      _stream_init (stream);
    }

  rc = mu_stream_flush (stream);
  if (rc)
    return rc;
  if (stream->shutdown)
    return stream->shutdown (stream, how);
  return 0;
}

int
mu_stream_set_flags (mu_stream_t stream, int fl)
{
  if (stream == NULL)
    return EINVAL;
  stream->flags |= (fl & ~_MU_STR_INTERN_MASK);
  return 0;
}

int
mu_stream_clr_flags (mu_stream_t stream, int fl)
{
  if (stream == NULL)
    return EINVAL;
  stream->flags &= ~(fl & ~_MU_STR_INTERN_MASK);
  return 0;
}

int
mu_stream_set_stat (mu_stream_t stream, int statmask,
		    mu_stream_stat_buffer statbuf)
{
  if (stream == NULL)
    return EINVAL;
  if (!statbuf)
    statmask = 0;
  stream->statmask = statmask;
  stream->statbuf = statbuf;
  if (stream->statbuf)
    memset (stream->statbuf, 0,
	    _MU_STREAM_STAT_MAX * sizeof (stream->statbuf[0]));
  return 0;
}

int
mu_stream_get_stat (mu_stream_t stream, int *pstatmask,
		    mu_off_t **pstatbuf)
{
  if (stream == NULL)
    return EINVAL;
  *pstatmask = stream->statmask;
  *pstatbuf = stream->statbuf;
  return 0;
}
  
