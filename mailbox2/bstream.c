/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

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

#include <stdlib.h>
#include <string.h>

#include <mailutils/monitor.h>
#include <mailutils/error.h>
#include <mailutils/sys/bstream.h>

static void
_stream_buffer_cleanup (void *arg)
{
  struct _stream_buffer *bs = arg;
  mu_refcount_unlock (bs->refcount);
}

static int
refill (struct _stream_buffer *bs, off_t offset)
{
  int status;
  if (bs->rbuffer.base == NULL)
    {
      bs->rbuffer.base = calloc (1, bs->rbuffer.bufsize);
      if (bs->rbuffer.base == NULL)
	return MU_ERROR_NO_MEMORY;
    }
  bs->rbuffer.ptr = bs->rbuffer.base;
  bs->rbuffer.count = 0;
  bs->rbuffer.offset = offset;
  status = stream_read (bs->stream, bs->rbuffer.ptr, bs->rbuffer.bufsize,
			offset, (size_t *)&(bs->rbuffer.count));
  return status;
}

int
_stream_buffer_ref (stream_t stream)
{
  struct _stream_buffer *bs = (struct _stream_buffer *)stream;
  return mu_refcount_inc (bs->refcount);
}

void
_stream_buffer_destroy (stream_t *pstream)
{
  struct _stream_buffer *bs = (struct _stream_buffer *)*pstream;
  if (mu_refcount_dec (bs->refcount) == 0)
    {
      stream_destroy (&bs->stream);
      mu_refcount_destroy (&bs->refcount);
      free (bs);
    }
}

int
_stream_buffer_open (stream_t stream, const char *name, int port, int flags)
{
  struct _stream_buffer *bs = (struct _stream_buffer *)stream;
  return stream_open (bs->stream, name, port, flags);
}

int
_stream_buffer_close (stream_t stream)
{
  struct _stream_buffer *bs = (struct _stream_buffer *)stream;
  mu_refcount_lock (bs->refcount);
  /* Clear the buffer of any residue left.  */
  if (bs->rbuffer.base && bs->rbuffer.bufsize)
    {
      bs->rbuffer.ptr = bs->rbuffer.base;
      bs->rbuffer.count = 0;
      memset (bs->rbuffer.base, '\0', bs->rbuffer.bufsize);
    }
  mu_refcount_unlock (bs->refcount);
  return stream_close (bs->stream);
}

/* We have to be clear about the buffering scheme, it is not designed to be
   use as a fully fledge buffer mechanism.  It is a simple mechanims for
   networking. Lots of code between POP and IMAP can be share this way.
   The buffering is on the read only, the writes fall through.  */
int
_stream_buffer_read (stream_t stream, void *buf, size_t count, off_t offset,
		     size_t *pnread)
{
  int status = 0;
  struct _stream_buffer *bs = (struct _stream_buffer *)stream;

  /* Noop.  */
  if (count == 0)
    {
      if (pnread)
	*pnread = 0;
    }
  else if (bs->rbuffer.bufsize == 0)
    {
      /* If rbuffer.bufsize == 0.  It means they did not want the buffer
	 mechanism, ... what are we doing here?  */
      status = stream_read (bs->stream, buf, count, offset, pnread);
    }
  else
    {
      size_t residue = count;
      char *p = buf;
      int r;

      mu_refcount_lock (bs->refcount);
      monitor_cleanup_push (_stream_buffer_cleanup, bs);

      /* If the amount requested is bigger then the buffer cache size
	 bypass it.  Do no waste time and let it through.  */
      if (count > bs->rbuffer.bufsize)
	{
	  r = 0;
	  /* Drain our buffer first.  */
	  if (bs->rbuffer.count > 0 && offset == bs->rbuffer.offset)
	    {
	      memcpy(p, bs->rbuffer.ptr, bs->rbuffer.count);
	      bs->rbuffer.offset += bs->rbuffer.count;
	      residue -= bs->rbuffer.count;
	      p += bs->rbuffer.count;
	      offset += bs->rbuffer.count;
	    }
	  bs->rbuffer.count = 0; /* Signal we will need to refill.  */
	  status = stream_read (bs->stream, p, residue, offset, &r);
	  bs->rbuffer.offset += r;
	  residue -= r;
	  if (pnread)
	    *pnread = count - residue;
	}
      else
	{
	  int done = 0;

	  /* Should not be necessary but guard never the less.  */
	  if (bs->rbuffer.count < 0)
	    bs->rbuffer.count = 0;

	  /* Drain the buffer, if we have less then requested.  */
	  while (residue > (size_t)(r = bs->rbuffer.count))
	    {
	      (void)memcpy (p, bs->rbuffer.ptr, (size_t)r);
	      bs->rbuffer.ptr += r;
	      bs->rbuffer.offset += r;
	      /* bs->rbuffer.count = 0 ... done in refill */
	      p += r;
	      residue -= r;
	      status = refill (bs, bs->rbuffer.offset);
	      /* Did we reach the end.  */
	      if (status != 0 || bs->rbuffer.count == 0)
		{
		  /* We have something in the buffer return the error on the
		     next call.  */
		  if (count != residue)
		    status = 0;
		  if (pnread)
		    *pnread = count - residue;
		  done = 1;
		  break;
		}
	    }
	  if (!done)
	    {
	      memcpy(p, bs->rbuffer.ptr, residue);
	      bs->rbuffer.count -= residue;
	      bs->rbuffer.ptr += residue;
	      bs->rbuffer.offset += residue;
	      if (pnread)
		*pnread = count;
	    }
	}
      mu_refcount_unlock (bs->refcount);
      monitor_cleanup_pop (0);
    }
  return status;
}

/*
 * Read at most n-1 characters.
 * Stop when a newline has been read, or the count runs out.
 */
int
_stream_buffer_readline (stream_t stream, char *buf, size_t count,
			 off_t offset, size_t *pnread)
{
  int status = 0;
  struct _stream_buffer *bs = (struct _stream_buffer *)stream;

  /* Noop.  */
  if (count == 0)
    {
      if (pnread)
	*pnread = 0;
    }
  else if (bs->rbuffer.bufsize == 0)
    {
      /* Use the provided readline.  */
      status = stream_readline (bs->stream, buf, count, offset, pnread);
    }
  else /* Buffered.  */
    {
      char *s = buf;
      char *p, *nl;
      size_t len;
      size_t total = 0;

      mu_refcount_lock (bs->refcount);
      monitor_cleanup_push (_stream_buffer_cleanup, bs);

      count--;  /* Leave space for the null.  */

      while (count != 0)
	{
	  /* If the buffer is empty refill it.  */
	  len = bs->rbuffer.count;
	  if (len <= 0)
	    {
	      status = refill (bs, offset);
	      if (status != 0 || bs->rbuffer.count == 0)
		{
		  break;
		}
	      len = bs->rbuffer.count;
	    }
	  p = bs->rbuffer.ptr;

	  /* Scan through at most n bytes of the current buffer,
	     looking for '\n'.  If found, copy up to and including
	     newline, and stop.  Otherwise, copy entire chunk
	     and loop.  */
	  if (len > count)
	    len = count;
	  nl = memchr (p, '\n', len);
	  if (nl != NULL)
	    {
	      len = ++nl - p;
	      bs->rbuffer.count -= len;
	      bs->rbuffer.ptr = nl;
	      bs->rbuffer.offset += len;
	      memcpy (s, p, len);
	      total += len;
	      s += len;
	      break;
	    }
	  bs->rbuffer.count -= len;
	  bs->rbuffer.ptr += len;
	  bs->rbuffer.offset += len;
	  memcpy(s, p, len);
	  total += len;
	  s += len;
	  count -= len;
        }
      *s = 0;
      if (pnread)
	*pnread = s - buf;

      mu_refcount_unlock (bs->refcount);
      monitor_cleanup_pop (0);
    }
  return status;
}

int
_stream_buffer_write (stream_t stream, const void *buf, size_t count,
		      off_t offset, size_t *pnwrite)
{
  struct _stream_buffer *bs = (struct _stream_buffer *)stream;
  return stream_write (bs->stream, buf, count, offset, pnwrite);
}

int
_stream_buffer_get_fd (stream_t stream, int *pfd)
{
  struct _stream_buffer *bs = (struct _stream_buffer *)stream;
  return stream_get_fd (bs->stream, pfd);
}

int
_stream_buffer_get_flags (stream_t stream, int *pfl)
{
  struct _stream_buffer *bs = (struct _stream_buffer *)stream;
  return stream_get_flags (bs->stream, pfl);
}

int
_stream_buffer_get_size (stream_t stream, off_t *psize)
{
  struct _stream_buffer *bs = (struct _stream_buffer *)stream;
  return stream_get_size (bs->stream, psize);
}

int
_stream_buffer_truncate (stream_t stream, off_t len)
{
  struct _stream_buffer *bs = (struct _stream_buffer *)stream;
  return stream_truncate (bs->stream, len);
}

int
_stream_buffer_flush (stream_t stream)
{
  struct _stream_buffer *bs = (struct _stream_buffer *)stream;
  return stream_flush (bs->stream);
}

int
_stream_buffer_get_state (stream_t stream, enum stream_state *pstate)
{
  struct _stream_buffer *bs = (struct _stream_buffer *)stream;
  return stream_get_state (bs->stream, pstate);
}

int
_stream_buffer_is_seekable (stream_t stream)
{
  struct _stream_buffer *bs = (struct _stream_buffer *)stream;
  return stream_is_seekable (bs->stream);
}

int
_stream_buffer_tell (stream_t stream, off_t *off)
{
  struct _stream_buffer *bs = (struct _stream_buffer *)stream;
  return stream_tell (bs->stream, off);
}

int
_stream_buffer_is_readready (stream_t stream, int timeout)
{
  struct _stream_buffer *bs = (struct _stream_buffer *)stream;
  /* Drain our buffer first.  */
  mu_refcount_lock (bs->refcount);
  if (bs->rbuffer.count > 0)
    {
      mu_refcount_unlock (bs->refcount);
      return 1;
    }
  mu_refcount_unlock (bs->refcount);
  return stream_is_readready (bs->stream, timeout);
}

int
_stream_buffer_is_writeready (stream_t stream, int timeout)
{
  struct _stream_buffer *bs = (struct _stream_buffer *)stream;
  return stream_is_writeready (bs->stream, timeout);
}

int
_stream_buffer_is_exceptionpending (stream_t stream, int timeout)
{
  struct _stream_buffer *bs = (struct _stream_buffer *)stream;
  return stream_is_exceptionpending (bs->stream, timeout);
}

int
_stream_buffer_is_open (stream_t stream)
{
  struct _stream_buffer *bs = (struct _stream_buffer *)stream;
  return stream_is_open (bs->stream);
}


static struct _stream_vtable _stream_buffer_vtable =
{
  _stream_buffer_ref,
  _stream_buffer_destroy,

  _stream_buffer_open,
  _stream_buffer_close,

  _stream_buffer_read,
  _stream_buffer_readline,
  _stream_buffer_write,

  _stream_buffer_tell,

  _stream_buffer_get_size,
  _stream_buffer_truncate,
  _stream_buffer_flush,

  _stream_buffer_get_fd,
  _stream_buffer_get_flags,
  _stream_buffer_get_state,

  _stream_buffer_is_seekable,
  _stream_buffer_is_readready,
  _stream_buffer_is_writeready,
  _stream_buffer_is_exceptionpending,

  _stream_buffer_is_open
};

int
_stream_buffer_ctor (struct _stream_buffer *bs, stream_t stream,
		     size_t bufsize)
{
  mu_refcount_create (&(bs->refcount));
  if (bs->refcount == NULL)
    return MU_ERROR_NO_MEMORY;

  bs->stream = stream;
  bs->rbuffer.bufsize = bufsize;
  bs->base.vtable = &_stream_buffer_vtable;
  return 0;
}

void
_stream_buffer_dtor (stream_t stream)
{
  struct _stream_buffer *bs = (struct _stream_buffer *)stream;
  if (bs)
    {
      stream_destroy (&bs->stream);
      mu_refcount_destroy (&bs->refcount);
    }
}

int
stream_buffer_create (stream_t *pstream, stream_t stream, size_t bufsize)
{
  struct _stream_buffer *bs;
  int status;

  if (pstream == NULL || stream == NULL || bufsize == 0)
    return MU_ERROR_INVALID_PARAMETER;

  bs = calloc (1, sizeof *bs);
  if (bs == NULL)
    return MU_ERROR_NO_MEMORY;

  status = _stream_buffer_ctor (bs, stream, bufsize);
  if (status != 0)
    return status;
  *pstream = &bs->base;
  return 0;
}
