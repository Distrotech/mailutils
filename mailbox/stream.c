/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Library Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

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

#include <stream0.h>

static int refill (stream_t, off_t);

/* Stream is a way for object to do I/O, they can take over(overload) the
   the read/write functions for there needs.  We are doing some very
   minimal buffering on the read when the stream_bufsiz is set, this
   unfortunately does not take to account the offset, this buffering is more
   for networking stream (POP/IMAP).  No buffering on the write. */
int
stream_create (stream_t *pstream, int flags, void *owner)
{
  stream_t stream;
  if (pstream == NULL || owner == NULL)
    return EINVAL;
  stream = calloc (1, sizeof (*stream));
  if (stream == NULL)
    return ENOMEM;
  stream->owner = owner;
  stream->flags = flags;
  /* By default unbuffered, the buffered scheme is not for all models, it
     really makes sense for networking streams, where there is no offset.  */
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
	  if (stream->_destroy)
	    stream->_destroy (stream);
	  if (stream->rbuffer.base)
	    free (stream->rbuffer.base);
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
stream_open (stream_t stream, const char *name, int port, int flags)
{
  if (stream == NULL)
    return EINVAL;
  stream->state = MU_STREAM_STATE_OPEN;
  stream->flags |= flags;
  if (stream->_open)
    return stream->_open (stream, name, port, flags);
  return  0;
}

int
stream_close (stream_t stream)
{
  if (stream == NULL)
    return EINVAL;
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
stream_setbufsiz (stream_t stream, size_t size)
{
  if (stream == NULL)
    return EINVAL;
  stream->rbuffer.bufsiz = size;
  return 0;
}

/* We have to be clear about the buffering scheme, it is not designed to be
   use as a fully fledge buffer mechanism.  It is rather turn for networking.
   Lots of code between POP and IMAP can be share this way.
   - First caveat; the offset as no meaning, it is up to the concrete
   _read() to return EISPIPE when error.
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
      int r;

      /* Fill the buffer, do not want to start empty hand.  */
      //      if (is->rbuffer.count <= 0 || offset < is->rbuffer.offset
      //  || offset > (is->rbuffer.offset + is->rbuffer.count))
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
	  (void)memcpy (buf, is->rbuffer.ptr, (size_t)r);
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
      (void)memcpy(buf, is->rbuffer.ptr, residue);
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

  if (count == 0)
    {
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
      //      if ((offset < is->rbuffer.offset
      //	   || offset > (is->rbuffer.offset + is->rbuffer.count)))
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
	      (void)memcpy ((void *)s, (void *)p, len);
	      total += len;
	      s[len] = 0;
	      //fprintf (stderr, ":%d %d:%s", len, total, s);
	      if (pnread)
		*pnread = total;
	      return 0;
	    }
	  is->rbuffer.count -= len;
	  is->rbuffer.ptr += len;
	  is->rbuffer.offset += len;
	  (void)memcpy((void *)s, (void *)p, len);
	  //fprintf (stderr, "!:%d %d\n", len, total);
	  total += len;
	  s += len;
	  count -= len;
        }
      *s = 0;
      //fprintf (stderr, "1:%s", s);
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
stream_get_fd (stream_t stream, int *pfd)
{
  if (stream == NULL || stream->_get_fd == NULL)
    return EINVAL;
  return stream->_get_fd (stream, pfd);
}

int
stream_get_flags (stream_t stream, int *pfl)
{
  if (stream == NULL && pfl == NULL )
    return EINVAL;
  *pfl = stream->flags;
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
  if (stream == NULL || pstate == NULL)
    return EINVAL;
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
	         int (*_open) (stream_t, const char *, int, int), void *owner)
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
stream_set_fd (stream_t stream, int (*_get_fd) (stream_t, int *), void *owner)
{
  if (stream == NULL)
    return EINVAL;
  if (owner == stream->owner)
    {
      stream->_get_fd = _get_fd;
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
      //fprintf (stderr, "COUNT%d\n", stream->rbuffer.count);
      //stream->rbuffer.ptr[stream->rbuffer.count] = 0;
      //fprintf (stderr, "%s\n", stream->rbuffer.ptr);
      return status;
    }
  return ENOTSUP;
}
