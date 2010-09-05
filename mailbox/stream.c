/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2009, 2010 Free Software Foundation, Inc.

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

static void
_stream_setflag (struct _mu_stream *stream, int flag)
{
  if (stream->event_cb && (stream->event_mask & flag))
    stream->event_cb (stream, _MU_STR_EVENT_SET, flag);
  stream->flags |= flag;
}

static void
_stream_clrflag (struct _mu_stream *stream, int flag)
{
  if (stream->event_cb && (stream->event_mask & flag))
    stream->event_cb (stream, _MU_STR_EVENT_CLR, flag);
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

#define _stream_advance_buffer(s,n) ((s)->cur += n, (s)->level -= n)
#define _stream_buffer_offset(s) ((s)->cur - (s)->buffer)
#define _stream_orig_level(s) ((s)->level + _stream_buffer_offset (s))
#define _stream_buffer_freespace(s) \
  ((s)->bufsize - (s)->level - _stream_buffer_offset(s))
#define _stream_buffer_is_full(s) (_stream_buffer_freespace(s) == 0)

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
      rc = mu_stream_read_unbuffered (stream,
				      stream->buffer, stream->bufsize,
				      0,
				      &stream->level);
      break;
	
    case mu_buffer_line:
      for (n = 0;
	   n < stream->bufsize
	     && (rc = mu_stream_read_unbuffered (stream,
						 &c, 1, 0, &rdn)) == 0;)
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
  stream->cur = stream->buffer;
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
	       || memchr (stream->cur, '\n', stream->level) != NULL;

      case mu_buffer_full:
	return _stream_buffer_is_full (stream);
      }
    return 0;
}

static int
_stream_flush_buffer (struct _mu_stream *stream, int all)
{
  int rc;
  char *end;
		  
  if (stream->flags & _MU_STR_DIRTY)
    {
      if ((stream->flags & MU_STREAM_SEEK)
	  && (rc = mu_stream_seek (stream, stream->offset, MU_SEEK_SET, NULL)))
	return rc;

      switch (stream->buftype)
	{
	case mu_buffer_none:
	    abort(); /* should not happen */
	    
	case mu_buffer_full:
	  if ((rc = mu_stream_write_unbuffered (stream, stream->cur,
						stream->level, 1, NULL)))
	    return rc;
	  _stream_advance_buffer (stream, stream->level);
	  break;
	    
	case mu_buffer_line:
	  if (stream->level == 0)
	    break;
	  for (end = memchr (stream->cur, '\n', stream->level);
	       end;
	       end = memchr (stream->cur, '\n', stream->level))
	    {
	      size_t size = end - stream->cur + 1;
	      rc = mu_stream_write_unbuffered (stream,
					       stream->cur,
					       size, 1, NULL);
	      if (rc)
		return rc;
	      _stream_advance_buffer (stream, size);
	    }
	  if ((all && stream->level) || _stream_buffer_is_full (stream))
	    {
	      rc = mu_stream_write_unbuffered (stream,
					       stream->cur,
					       stream->level,
					       1, NULL);
	      if (rc)
		return rc;
	      _stream_advance_buffer (stream, stream->level);
	    }
	}
    }
  else if (all)
    _stream_advance_buffer (stream, stream->level);
  
  if (stream->level)
    {
      if (stream->cur > stream->buffer)
	memmove (stream->buffer, stream->cur, stream->level);
    }
  else
    {
      _stream_clrflag (stream, _MU_STR_DIRTY);
      stream->level = 0;
    }
  stream->cur = stream->buffer;
  return 0;
}


mu_stream_t
_mu_stream_create (size_t size, int flags)
{
  struct _mu_stream *str;
  if (size < sizeof (str))
    abort ();
  str = mu_zalloc (size);
  str->flags = flags;
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

int
mu_stream_open (mu_stream_t stream)
{
  int rc;

  if (stream->open && (rc = stream->open (stream)))
    return mu_stream_seterr (stream, rc, 1);
  stream->bytes_in = stream->bytes_out = 0;
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
  return stream->flags & _MU_STR_EOF;
}

int
mu_stream_seek (mu_stream_t stream, mu_off_t offset, int whence,
		mu_off_t *pres)
{    
  int rc;
  mu_off_t size;
  
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
	  *pres = stream->offset + _stream_buffer_offset (stream);
	  return 0;
	}
      offset += stream->offset;
      break;

    case MU_SEEK_END:
      rc = mu_stream_size (stream, &size);
      if (rc)
	return mu_stream_seterr (stream, rc, 1);
      offset += size;
      break;

    default:
      return mu_stream_seterr (stream, EINVAL, 1);
    }

  if (stream->buftype == mu_buffer_none
      || offset < stream->offset
      || offset > stream->offset + _stream_buffer_offset (stream))
    {
      if ((rc = _stream_flush_buffer (stream, 1)))
	return rc;
      rc = stream->seek (stream, offset, &stream->offset);
      if (rc == ESPIPE)
	return rc;
      if (rc)
	return mu_stream_seterr (stream, rc, 1);
      _mu_stream_cleareof (stream);
    }
  
  if (pres)
    *pres = stream->offset + _stream_buffer_offset (stream);
  return 0;
}

/* Skip COUNT bytes from the current position in stream by reading from
   it.  Return new offset in PRES.

   Return 0 on success, EACCES if STREAM was not opened for reading.
   Another non-zero exit codes are propagated from the underlying
   input operations.
   
   This function is designed to help implement seek method in otherwise
   unseekable streams (such as filters).  Do not use it if you absolutely
   have to.  Using it on an unbuffered stream is a terrible waste of CPU. */
int
mu_stream_skip_input_bytes (mu_stream_t stream, mu_off_t count, mu_off_t *pres)
{
  mu_off_t pos;
  int rc;

  if (!(stream->flags & MU_STREAM_READ))
    return mu_stream_seterr (stream, EACCES, 1);

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
	  if ((rc = _stream_flush_buffer (stream, 1)))
	    return rc;
	  if (stream->level == 0)
	    {
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
	      size_t delta = count - pos;
	      _stream_advance_buffer (stream, delta);
	      pos = count;
	      rc = 0;
	      break;
	    }
	  pos += stream->level;
	}
    }
  
  stream->offset += pos;
  if (pres)
    *pres = stream->offset;
  return rc;
}

int
mu_stream_set_buffer (mu_stream_t stream, enum mu_buffer_type type,
		      size_t size)
{
  if (size == 0)
    type = mu_buffer_none;

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
  stream->cur = stream->buffer;
  stream->level = 0;
    
  return 0;
}

int
mu_stream_read_unbuffered (mu_stream_t stream, void *buf, size_t size,
			   int full_read,
			   size_t *pnread)
{
  int rc;
  size_t nread;
    
  if (!stream->read) 
    return mu_stream_seterr (stream, ENOSYS, 0);

  if (!(stream->flags & MU_STREAM_READ)) 
    return mu_stream_seterr (stream, EACCES, 1);
    
  if (stream->flags & _MU_STR_ERR)
    return stream->last_err;
    
  if ((stream->flags & _MU_STR_EOF) || size == 0)
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
	       && (rc = stream->read (stream, buf, size, &rdbytes)) == 0)
	  {
	    if (rdbytes == 0)
	      {
		_stream_setflag (stream, _MU_STR_EOF);
		break;
	      }
	    buf += rdbytes;
	    nread += rdbytes;
	    size -= rdbytes;
	    stream->bytes_in += rdbytes;
	    
	  }
	if (size && rc)
	  rc = mu_stream_seterr (stream, rc, 0);
      }
    else
      {
	rc = stream->read (stream, buf, size, &nread);
	if (rc == 0)
	  {
	    if (nread == 0)
	      _stream_setflag (stream, _MU_STR_EOF);
	    stream->bytes_in += nread;
	  }
	mu_stream_seterr (stream, rc, rc != 0);
      }
    stream->offset += nread;
    if (pnread)
      *pnread = nread;
    
    return rc;
}

int
mu_stream_write_unbuffered (mu_stream_t stream,
			    const void *buf, size_t size,
			    int full_write,
			    size_t *pnwritten)
{
  int rc;
  size_t nwritten;
  
  if (!stream->write) 
    return mu_stream_seterr (stream, ENOSYS, 0);

  if (!(stream->flags & MU_STREAM_WRITE)) 
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
	     && (rc = stream->write (stream, bufp, size, &wrbytes))
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
	  stream->bytes_out += wrbytes;
	}
    }
  else
    {
      rc = stream->write (stream, buf, size, &nwritten);
      if (rc == 0)
	stream->bytes_out += nwritten;
    }
  _stream_setflag (stream, _MU_STR_WRT);
  stream->offset += nwritten;
  if (pnwritten)
    *pnwritten = nwritten;
  mu_stream_seterr (stream, rc, rc != 0);
  return rc;
}

int
mu_stream_read (mu_stream_t stream, void *buf, size_t size, size_t *pread)
{
  if (stream->buftype == mu_buffer_none)
    return mu_stream_read_unbuffered (stream, buf, size, !pread, pread);
  else
    {
      char *bufp = buf;
      size_t nbytes = 0;
      while (size)
	{
	  size_t n;
	  int rc;
	  
	  if (stream->level == 0)
	    {
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
	  if (n > stream->level)
	    n = stream->level;
	  memcpy (bufp, stream->cur, n);
	  _stream_advance_buffer (stream, n);
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
      char *p;
      size_t len;
      
      if (stream->level == 0)
	{
	  if ((rc = _stream_fill_buffer (stream)) || stream->level == 0)
	    break;
	}
      
      p = memchr (stream->cur, delim, stream->level);
      len = p ? p - stream->cur + 1 : stream->level;
      if (len > size)
	len = size;
      memcpy (buf, stream->cur, len);
      _stream_advance_buffer (stream, len);
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
  
  if (size == 0)
    return EINVAL;
    
  if (stream->readdelim)
    rc = stream->readdelim (stream, buf, size, delim, pread);
  else if (stream->buftype != mu_buffer_none)
    rc = _stream_scandelim (stream, buf, size, delim, pread);
  else
    rc = _stream_readdelim (stream, buf, size, delim, pread);
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
  
  if (stream->buftype == mu_buffer_none)
    rc = mu_stream_write_unbuffered (stream, buf, size,
				     !pnwritten, pnwritten);
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
	  memcpy (stream->cur + stream->level, bufp, n);
	  stream->level += n;
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
  rc = _stream_flush_buffer (stream, 1);
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
  mu_stream_flush (stream);
  /* Do close the stream only if it is not used by anyone else */
  if (stream->ref_count > 1)
    return 0;
  if (stream->close)
    rc = stream->close (stream);
  return rc;
}

int
mu_stream_size (mu_stream_t stream, mu_off_t *psize)
{
  int rc;
    
  if (!stream->size)
    return mu_stream_seterr (stream, ENOSYS, 0);
  rc = stream->size (stream, psize);
  return mu_stream_seterr (stream, rc, rc != 0);
}

mu_off_t
mu_stream_bytes_in (mu_stream_t stream)
{
  return stream->bytes_in;
}

mu_off_t
mu_stream_bytes_out (mu_stream_t stream)
{
  return stream->bytes_out;
}

int
mu_stream_ioctl (mu_stream_t stream, int code, void *ptr)
{
  if (stream->ctl == NULL)
    return ENOSYS;
  return stream->ctl (stream, code, ptr);
}

int
mu_stream_wait (mu_stream_t stream, int *pflags, struct timeval *tvp)
{
  int flg = 0;
  if (stream == NULL)
    return EINVAL;
  
  /* Take to acount if we have any buffering.  */
  /* FIXME: How about MU_STREAM_READY_WR? */
  if ((*pflags) & MU_STREAM_READY_RD 
      && stream->buftype != mu_buffer_none
      && stream->level > 0)
    {
      flg = MU_STREAM_READY_RD;
      *pflags &= ~MU_STREAM_READY_RD;
    }

  if (stream->wait)
    {
      int rc = stream->wait (stream, pflags, tvp);
      if (rc == 0)
	*pflags |= flg;
      return rc;
    }
  
  return ENOSYS;
}

int
mu_stream_truncate (mu_stream_t stream, mu_off_t size)
{
  if (stream->truncate)
    return stream->truncate (stream, size);
  return ENOSYS;
}

int
mu_stream_shutdown (mu_stream_t stream, int how)
{
  if (stream->shutdown)
    return stream->shutdown (stream, how);
  return ENOSYS;
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

static void
swapstr (mu_stream_t stream, mu_stream_t *curstr, mu_stream_t *newstr)
{
  mu_stream_t tmp;

  tmp = *newstr;
  *newstr = *curstr;
  *curstr = tmp;
  if (!(stream->flags & MU_STREAM_AUTOCLOSE))
    {
      if (*newstr)
	mu_stream_unref (*newstr);
      if (tmp)
	mu_stream_ref (tmp);
    }
  if (!tmp)
    mu_stream_seterr (stream, MU_ERR_NO_TRANSPORT, 1);
  else if (stream->last_err == MU_ERR_NO_TRANSPORT)
    mu_stream_clearerr (stream);
}

static int
swapstr_recursive (mu_stream_t stream, mu_stream_t *curstr,
		   mu_stream_t *newstr, int flags)
{
  mu_stream_t strtab[2];
  int rc = ENOSYS;

  if (*curstr == NULL && *newstr == NULL)
    return 0;
  
  if (*curstr)
    {
      strtab[0] = *newstr;
      strtab[1] = NULL;
      rc = mu_stream_ioctl (*curstr, MU_IOCTL_SWAP_STREAM, strtab);
      if (rc)
	{
	  if ((flags & _MU_SWAP_IOCTL_MUST_SUCCEED)
	      || !(rc == ENOSYS || rc == EINVAL))
	    return rc;
	}
    }
  if (rc == 0)
    *newstr = strtab[0];
  else
    swapstr (stream, curstr, newstr);
  return 0;
}

/* CURTRANS[2] contains I/O transport streams used by STREAM,
   NEWTRANS[2] contains another pair of streams.
   This function swaps the items of these two arrays using the
   MU_IOCTL_SWAP_STREAM ioctl.  It is intended for use by STREAM's
   ioctl method and is currently used by iostream.c */
   
int
_mu_stream_swap_streams (mu_stream_t stream, mu_stream_t *curtrans,
			 mu_stream_t *newtrans, int flags)
{
  int rc;

  rc = swapstr_recursive (stream, &curtrans[0], &newtrans[0], flags);
  if (rc)
    return rc;
  if (flags & _MU_SWAP_FIRST_ONLY)
    return 0;
  rc = swapstr_recursive (stream, &curtrans[1], &newtrans[1], flags);
  if (rc)
    {
      int rc1 = swapstr_recursive (stream, &curtrans[0], &newtrans[0], flags);
      if (rc1)
	{
	  mu_diag_output (MU_DIAG_CRIT,
			  _("restoring streams on %p failed: %s"),
			  stream, mu_strerror (rc1));
	  abort ();
	}
    }
  return rc;
}
  



