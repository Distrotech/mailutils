/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2004 Free Software Foundation, Inc.

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


/* Credits.  Some of the Readline an buffering scheme was taken
   from 4.4BSDLite2.

   Copyright (c) 1990, 1993
   The Regents of the University of California.  All rights reserved.

   This code is derived from software contributed to Berkeley by
   Chris Torek.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <mailutils/property.h>
#include <mailutils/errno.h>
#include <stream0.h>

static int refill (stream_t, off_t);

/* A stream provides a way for an object to do I/O. It overloads
   stream read/write functions. Only a minimal buffering is done
   and that if stream's bufsiz member is set. If the requested
   offset does not equal the one maintained internally the buffer
   is flushed and refilled. This buffering scheme is more convenient
   for networking streams (POP/IMAP).
   Writes are always unbuffered. */
int
stream_create (stream_t *pstream, int flags, void *owner)
{
  stream_t stream;
  if (pstream == NULL)
    return MU_ERR_OUT_PTR_NULL;
  if (owner == NULL)
    return EINVAL;
  stream = calloc (1, sizeof (*stream));
  if (stream == NULL)
    return ENOMEM;
  stream->owner = owner;
  stream->flags = flags;
  /* By default unbuffered, the buffering scheme is not for all models, it
     really makes sense for network streams, where there is no offset.  */
  /* stream->rbuffer.bufsiz = BUFSIZ; */
  *pstream = stream;
  return 0;
}

void
stream_destroy (stream_t *pstream, void *owner)
{
   if (pstream && *pstream)
    {
      stream_t stream = *pstream;
      if ((stream->flags & MU_STREAM_NO_CHECK) || stream->owner == owner)
	{
	  stream_close(stream);
	  if (stream->rbuffer.base)
	    free (stream->rbuffer.base);

	  if (stream->_destroy) 
	    stream->_destroy (stream);

	  free (stream);
	}
      *pstream = NULL;
    }
}

void *
stream_get_owner (stream_t stream)
{
  return (stream) ? stream->owner : NULL;
}

int
stream_open (stream_t stream)
{
  if (stream == NULL)
    return EINVAL;
  stream->state = MU_STREAM_STATE_OPEN;

  if (stream->_open)
    return stream->_open (stream);
  return  0;
}

int
stream_close (stream_t stream)
{
  if (stream == NULL)
    return EINVAL;
  if (stream->state == MU_STREAM_STATE_CLOSE)
    return 0;
  
  stream->state = MU_STREAM_STATE_CLOSE;
  /* Clear the buffer of any residue left.  */
  if (stream->rbuffer.base)
    {
      stream->rbuffer.ptr = stream->rbuffer.base;
      stream->rbuffer.count = 0;
      memset (stream->rbuffer.base, '\0', stream->rbuffer.bufsiz);
    }
  if (stream->_close)
    return stream->_close (stream);
  return  0;
}

int
stream_is_seekable (stream_t stream)
{
  return (stream) ? stream->flags & MU_STREAM_SEEKABLE : 0;
}

int
stream_setbufsiz (stream_t stream, size_t size)
{
  if (stream == NULL)
    return EINVAL;
  stream->rbuffer.bufsiz = size;
  return 0;
}

/* We have to be clear about the buffering scheme, it is not designed to be
   used as a full-fledged buffer mechanism.  It is a simple mechanism for
   networking. Lots of code between POP and IMAP can be shared this way.
   - First caveat; the code maintains its own offset (rbuffer.offset member)
   and if it does not match the requested one, the data is flushed
   and the underlying _read is called. It is up to the latter to return
   EISPIPE when appropriate.
   - Again, this is targeting networking stream to make readline()
   a little bit more efficient, instead of reading a char at a time.  */

int
stream_read (stream_t is, char *buf, size_t count,
	     off_t offset, size_t *pnread)
{
  int status = 0;
  if (is == NULL || is->_read == NULL)
    return EINVAL;

  is->state = MU_STREAM_STATE_READ;

  /* Sanity check; noop.  */
  if (count == 0)
    {
      if (pnread)
	*pnread = 0;
      return 0;
    }

  /* If rbuffer.bufsiz == 0.  It means they did not want the buffer
     mechanism.  Good for them.  */
  if (is->rbuffer.bufsiz == 0)
    status = is->_read (is, buf, count, offset, pnread);
  else
    {
      size_t residue = count;
      size_t r;

      /* If the amount requested is bigger than the buffer cache size,
	 bypass it.  Do no waste time and let it through.  */
      if (count > is->rbuffer.bufsiz)
	{
	  r = 0;
	  /* Drain the buffer first.  */
	  if (is->rbuffer.count > 0 && offset == is->rbuffer.offset)
	    {
	      memcpy(buf, is->rbuffer.ptr, is->rbuffer.count);
	      is->rbuffer.offset += is->rbuffer.count;
	      residue -= is->rbuffer.count;
	      buf += is->rbuffer.count;
	      offset += is->rbuffer.count;
	    }
	  is->rbuffer.count = 0;
	  status = is->_read (is, buf, residue, offset, &r);
	  is->rbuffer.offset += r;
	  residue -= r;
	  if (pnread)
	    *pnread = count - residue;
	  return status;
	}

      /* Fill the buffer, do not want to start empty hand.  */
      if (is->rbuffer.count <= 0 || offset != is->rbuffer.offset)
	{
	  status = refill (is, offset);
	  if (status != 0)
	    return status;
	  /* Reached the end ??  */
	  if (is->rbuffer.count == 0)
	    {
	      if (pnread)
		*pnread = 0;
	      return status;
	    }
	}

      /* Drain the buffer, if we have less then requested.  */
      while (residue > (size_t)(r = is->rbuffer.count))
	{
	  memcpy (buf, is->rbuffer.ptr, (size_t)r);
	  /* stream->rbuffer.count = 0 ... done in refill */
	  is->rbuffer.ptr += r;
	  is->rbuffer.offset += r;
	  buf += r;
	  residue -= r;
	  status = refill (is, is->rbuffer.offset);
	  if (status != 0)
	    {
	      /* We have something in the buffer return the error on the
		 next call .  */
	      if (count != residue)
		{
		  if (pnread)
		    *pnread = count - residue;
		  status = 0;
		}
	      return status;
	    }
	  /* Did we reach the end.  */
	  if (is->rbuffer.count == 0)
	    {
	      if (pnread)
		*pnread = count - residue;
	      return status;
	    }
	}
      memcpy(buf, is->rbuffer.ptr, residue);
      is->rbuffer.count -= residue;
      is->rbuffer.ptr += residue;
      is->rbuffer.offset += residue;
      if (pnread)
	*pnread = count;
    }
  return status;
}

/*
 * Read at most n-1 characters.
 * Stop when a newline has been read, or the count runs out.
 */
int
stream_readline (stream_t is, char *buf, size_t count,
		 off_t offset, size_t *pnread)
{
  int status = 0;

  if (is == NULL)
    return EINVAL;

  is->state = MU_STREAM_STATE_READ;

  switch (count)
    {
    case 1:
      /* why would they do a thing like that?
	 stream_readline() is __always null terminated.  */
      if (buf)
	*buf = '\0';
    case 0: /* Buffer is empty noop.  */
      if (pnread)
	*pnread = 0;
      return 0;
    }

  /* Use the provided readline.  */
  if (is->rbuffer.bufsiz == 0 &&  is->_readline != NULL)
    status = is->_readline (is, buf, count, offset, pnread);
  else if (is->rbuffer.bufsiz == 0) /* No Buffering.  */
    {
      size_t n, nr = 0;
      char c;
      /* Grossly inefficient hopefully they override this */
      for (n = 1; n < count; n++)
	{
	  status = is->_read (is, &c, 1, offset, &nr);
	  if (status != 0) /* Error.  */
	    return status;
	  else if (nr == 1)
	    {
	      *buf++ = c;
	      offset++;
	      if (c == '\n') /* Newline is stored like fgets().  */
		break;
	    }
	  else if (nr == 0)
	    {
	      if (n == 1) /* EOF, no data read.  */
		n = 0;
	      break; /* EOF, some data was read.  */
	    }
	}
      *buf = '\0';
      if (pnread)
	*pnread = (n == count) ? n - 1: n;
    }
  else /* Buffered.  */
    {
      char *s = buf;
      char *p, *nl;
      size_t len;
      size_t total = 0;

      count--;  /* Leave space for the null.  */

      /* If out of range refill.  */
      /*      if ((offset < is->rbuffer.offset */
      /*	   || offset > (is->rbuffer.offset + is->rbuffer.count))) */
      if (offset != is->rbuffer.offset)
	{
	  status = refill (is, offset);
	  if (status != 0)
	    return status;
	  if (is->rbuffer.count == 0)
	    {
	      if (pnread)
		*pnread = 0;
	      return 0;
	    }
	}

      while (count != 0)
	{
	  /* If the buffer is empty refill it.  */
	  len = is->rbuffer.count;
	  if (len <= 0)
	    {
	      status = refill (is, is->rbuffer.offset);
	      if (status != 0)
		{
		  if (s != buf)
		    break;
		}
	      len = is->rbuffer.count;
	      if (len == 0)
		break;
	    }
	  p = is->rbuffer.ptr;

	  /* Scan through at most n bytes of the current buffer,
	     looking for '\n'.  If found, copy up to and including
	     newline, and stop.  Otherwise, copy entire chunk
	     and loop.  */
	  if (len > count)
	    len = count;
	  nl = memchr ((void *)p, '\n', len);
	  if (nl != NULL)
	    {
	      len = ++nl - p;
	      is->rbuffer.count -= len;
	      is->rbuffer.ptr = nl;
	      is->rbuffer.offset += len;
	      memcpy ((void *)s, (void *)p, len);
	      total += len;
	      s[len] = 0;
	      if (pnread)
		*pnread = total;
	      return 0;
	    }
	  is->rbuffer.count -= len;
	  is->rbuffer.ptr += len;
	  is->rbuffer.offset += len;
	  memcpy((void *)s, (void *)p, len);
	  total += len;
	  s += len;
	  count -= len;
        }
      *s = 0;
      if (pnread)
	*pnread = s - buf;
    }
  return status;
}

int
stream_write (stream_t os, const char *buf, size_t count,
	      off_t offset, size_t *pnwrite)
{
  int nleft;
  int err = 0;
  size_t nwriten = 0;
  size_t total = 0;

  if (os == NULL || os->_write == NULL)
      return EINVAL;
  os->state = MU_STREAM_STATE_WRITE;

  nleft = count;
  /* First try to send it all.  */
  while (nleft > 0)
    {
      err = os->_write (os, buf, nleft, offset, &nwriten);
      if (err != 0 || nwriten == 0)
        break;
      nleft -= nwriten;
      total += nwriten;
      buf += nwriten;
    }
  if (pnwrite)
    *pnwrite = total;
  return err;
}

int
stream_get_transport2 (stream_t stream,
		       mu_transport_t *p1, mu_transport_t *p2)
{
  if (stream == NULL || stream->_get_transport2 == NULL)
    return EINVAL;
  return stream->_get_transport2 (stream, p1, p2);
}

int
stream_get_transport (stream_t stream,
		      mu_transport_t *pt)
{
  return stream_get_transport2 (stream, pt, NULL);
}

int
stream_get_flags (stream_t stream, int *pfl)
{
  if (stream == NULL)
    return EINVAL;
  if (pfl == NULL)
    return MU_ERR_OUT_NULL;
  *pfl = stream->flags;
  return 0;
}

int
stream_set_property (stream_t stream, property_t property, void *owner)
{
  if (stream == NULL)
    return EINVAL;
  if (stream->owner != owner)
    return EACCES;
  if (stream->property)
    property_destroy (&(stream->property), stream);
  stream->property = property;
  return 0;
}

int
stream_get_property (stream_t stream, property_t *pp)
{
  if (stream == NULL)
    return EINVAL;
  if (stream->property == NULL)
    {
      int status = property_create (&(stream->property), stream);
      if (status != 0)
	return status;
    }
  *pp = stream->property;
  return 0;
}

int
stream_size (stream_t stream, off_t *psize)
{
  if (stream == NULL || stream->_size == NULL)
    return EINVAL;
  return stream->_size (stream, psize);
}

int
stream_truncate (stream_t stream, off_t len)
{
  if (stream == NULL || stream->_truncate == NULL )
    return EINVAL;

  return stream->_truncate (stream, len);
}


int
stream_flush (stream_t stream)
{
  if (stream == NULL || stream->_flush == NULL)
    return EINVAL;
  return stream->_flush (stream);
}


int
stream_get_state (stream_t stream, int *pstate)
{
  if (stream == NULL)
    return EINVAL;
  if (pstate == NULL)
    return MU_ERR_OUT_PTR_NULL;
  *pstate = stream->state;
  return 0;
}

int
stream_set_destroy (stream_t stream, void (*_destroy) (stream_t),  void *owner)
{
  if (stream == NULL)
    return EINVAL;

  if (stream->owner != owner)
    return EACCES;

  stream->_destroy = _destroy;
  return 0;
}

int
stream_set_open (stream_t stream,
	         int (*_open) (stream_t), void *owner)
{
  if (stream == NULL)
    return EINVAL;
  if (owner == stream->owner)
    {
      stream->_open = _open;
      return 0;
    }
  return EACCES;
}

int
stream_set_close (stream_t stream, int (*_close) (stream_t), void *owner)
{
  if (stream == NULL)
    return EINVAL;
  if (owner == stream->owner)
    {
      stream->_close = _close;
      return 0;
    }
  return EACCES;
}

int
stream_set_get_transport2 (stream_t stream,
			   int (*_get_trans) (stream_t, mu_transport_t *, mu_transport_t *),
			   void *owner)
{
  if (stream == NULL)
    return EINVAL;
  if (owner == stream->owner)
    {
      stream->_get_transport2 = _get_trans;
      return 0;
    }
  return EACCES;
}

int
stream_set_read (stream_t stream, int (*_read)
		 (stream_t, char *, size_t, off_t, size_t *),
		 void *owner)
{
  if (stream == NULL)
    return EINVAL;
  if (owner == stream->owner)
    {
      stream->_read = _read;
      return 0;
    }
  return EACCES;
}

int
stream_set_readline (stream_t stream, int (*_readline)
		 (stream_t, char *, size_t, off_t, size_t *),
		 void *owner)
{
  if (stream == NULL)
    return EINVAL;
  if (owner == stream->owner)
    {
      stream->_readline = _readline;
      return 0;
    }
  return EACCES;
}

int
stream_set_write (stream_t stream, int (*_write)
		  __P ((stream_t, const char *, size_t, off_t, size_t *)),
		  void *owner)
{
  if (stream == NULL)
    return EINVAL;
  if (stream->owner == owner)
    {
      stream->_write = _write;
      return 0;
    }
  return EACCES;
}


int
stream_set_size (stream_t stream, int (*_size)(stream_t, off_t *), void *owner)
{
  if (stream == NULL)
    return EINVAL;
  if (stream->owner != owner)
    return EACCES;
  stream->_size = _size;
  return 0;
}

int
stream_set_truncate (stream_t stream, int (*_truncate) (stream_t, off_t),
		     void *owner)
{
  if (stream == NULL)
    return EINVAL;
  if (stream->owner != owner)
    return EACCES;
  stream->_truncate = _truncate;
  return 0;
}

int
stream_set_flush (stream_t stream, int (*_flush) (stream_t), void *owner)
{
  if (stream == NULL)
    return EINVAL;
  if (stream->owner != owner)
    return EACCES;
  stream->_flush = _flush;
  return 0;
}

int
stream_set_flags (stream_t stream, int fl)
{
  if (stream == NULL)
    return EINVAL;
  stream->flags |= fl;
  return 0;
}

int
stream_set_strerror (stream_t stream,
		     int (*fp) (stream_t, char **), void *owner)
{
  if (stream == NULL)
    return EINVAL;
  if (stream->owner != owner)
    return EACCES;
  stream->_strerror = fp;
  return 0;
}

int
stream_set_wait (stream_t stream,
		 int (*wait) (stream_t, int *, struct timeval *), void *owner)
{
  if (stream == NULL)
    return EINVAL;
  if (stream->owner != owner)
    return EACCES;
  stream->_wait = wait;
  return 0;
}

int
stream_sequential_read (stream_t stream, char *buf, size_t size,
			size_t *nbytes)
{
  size_t rdbytes;
  int rc = stream_read (stream, buf, size, stream->offset, &rdbytes);
  if (!rc)
    {
      stream->offset += rdbytes;
      if (nbytes)
	*nbytes = rdbytes;
    }
  return rc;
}

int
stream_sequential_readline (stream_t stream, char *buf, size_t size,
			    size_t *nbytes)
{
  size_t rdbytes;
  int rc = stream_readline (stream, buf, size, stream->offset, &rdbytes);
  if (!rc)
    {
      stream->offset += rdbytes;
      if (nbytes)
	*nbytes = rdbytes;
    }
  return rc;
}

int
stream_sequential_write (stream_t stream, char *buf, size_t size)
{
  if (stream == NULL)
    return EINVAL;
  while (size > 0)
    {
      size_t sz;
      int rc = stream_write (stream, buf, size, stream->offset, &sz);
      if (rc)
	return rc;

      buf += sz;
      size -= sz;
      stream->offset += sz;
    }
  return 0;
}

int
stream_seek (stream_t stream, off_t off, int whence)
{
  off_t size = 0;
  size_t pos;
  int rc;
  
  if ((rc = stream_size (stream, &size)))
    return rc;
  
  switch (whence)
    {
    case SEEK_SET:
      pos = off;
      break;
      
    case SEEK_CUR:
      pos = off + stream->offset;
      break;
      
    case SEEK_END:
      pos = size + off;
      break;
      
    default:
      return EINVAL;
    }

  if (pos > size)
    return EIO;
  
  stream->offset = pos;
  return 0;
}

int
stream_wait (stream_t stream, int *pflags, struct timeval *tvp)
{
  if (stream == NULL)
    return EINVAL;
  if (stream->_wait)
    return stream->_wait (stream, pflags, tvp);
  return ENOSYS;
}

int
stream_strerror (stream_t stream, char **p)
{
  if (stream == NULL)
    return EINVAL;
  if (stream->_strerror)
    return stream->_strerror (stream, p);
  return ENOSYS;
}

static int
refill (stream_t stream, off_t offset)
{
  if (stream->_read)
    {
      int status;
      if (stream->rbuffer.base == NULL)
	{
	  stream->rbuffer.base = calloc (1, stream->rbuffer.bufsiz);
	  if (stream->rbuffer.base == NULL)
	    return ENOMEM;
	}
      stream->rbuffer.ptr = stream->rbuffer.base;
      stream->rbuffer.offset = offset;
      stream->rbuffer.count = 0;
      status = stream->_read (stream, stream->rbuffer.ptr,
			      stream->rbuffer.bufsiz, offset,
			      (size_t *)&(stream->rbuffer.count));
      return status;
    }
  return ENOSYS;
}
