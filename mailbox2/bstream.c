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

#include <stdlib.h>
#include <string.h>

#include <mailutils/monitor.h>
#include <mailutils/error.h>
#include <mailutils/sys/bstream.h>

static void
_bs_cleanup (void *arg)
{
  struct _bs *bs = arg;
  monitor_unlock (bs->lock);
}

static int
refill (struct _bs *bs)
{
  int status;
  if (bs->rbuffer.base == NULL)
    {
      bs->rbuffer.base = calloc (1, bs->rbuffer.bufsize);
      if (bs->rbuffer.base == NULL)
	return ENOMEM;
    }
  bs->rbuffer.ptr = bs->rbuffer.base;
  bs->rbuffer.count = 0;
  status = stream_read (bs->stream, bs->rbuffer.ptr, bs->rbuffer.bufsize,
			(size_t *)&(bs->rbuffer.count));
  return status;
}

static int
_bs_add_ref (stream_t stream)
{
  struct _bs *bs = (struct _bs *)stream;
  return stream_add_ref (bs->stream);
}

static int
_bs_destroy (stream_t stream)
{
  struct _bs *bs = (struct _bs *)stream;
  stream_destroy (bs->stream);
  monitor_destroy (bs->lock);
  free (bs);
  return 0;
}

static int
_bs_release (stream_t stream)
{
  struct _bs *bs = (struct _bs *)stream;
  int status = stream_release (bs->stream);
  if (status == 0)
    {
      _bs_destroy (stream);
      return 0;
    }
  return status;
}

static int
_bs_open (stream_t stream, const char *name, int port, int flags)
{
  struct _bs *bs = (struct _bs *)stream;
  return stream_open (bs->stream, name, port, flags);
}

static int
_bs_close (stream_t stream)
{
  struct _bs *bs = (struct _bs *)stream;
  monitor_lock (bs->lock);
  /* Clear the buffer of any residue left.  */
  if (bs->rbuffer.base && bs->rbuffer.bufsize)
    {
      bs->rbuffer.ptr = bs->rbuffer.base;
      bs->rbuffer.count = 0;
      memset (bs->rbuffer.base, '\0', bs->rbuffer.bufsize);
    }
  monitor_unlock (bs->lock);
  return stream_close (bs->stream);
}

/* We have to be clear about the buffering scheme, it is not designed to be
   use as a fully fledge buffer mechanism.  It is a simple mechanims for
   networking. Lots of code between POP and IMAP can be share this way.
   The buffering is on the read only, the writes fall through.  */
static int
_bs_read (stream_t stream, char *buf, size_t count, size_t *pnread)
{
  int status = 0;
  struct _bs *bs = (struct _bs *)stream;

  /* Noop.  */
  if (count == 0)
    {
      if (pnread)
	*pnread = 0;
    }
  else if (bs->rbuffer.bufsize == 0)
    {
      /* If rbuffer.bufsize == 0.  It means they did not want the buffer
	 mechanism, then what are we doing here?  */
      status = stream_read (bs->stream, buf, count, pnread);
    }
  else
    {
      size_t residue = count;
      int r;

      monitor_lock (bs->lock);
      monitor_cleanup_push (_bs_cleanup, bs);

      /* If the amount requested is bigger then the buffer cache size
	 bypass it.  Do no waste time and let it through.  */
      if (count > bs->rbuffer.bufsize)
	{
	  r = 0;
	  /* Drain our buffer first.  */
	  if (bs->rbuffer.count > 0)
	    {
	      memcpy(buf, bs->rbuffer.ptr, bs->rbuffer.count);
	      residue -= bs->rbuffer.count;
	      buf += bs->rbuffer.count;
	    }
	  bs->rbuffer.count = 0; /* Signal we will need to refill.  */
	  status = stream_read (bs->stream, buf, residue, &r);
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
	      (void)memcpy (buf, bs->rbuffer.ptr, (size_t)r);
	      bs->rbuffer.ptr += r;
	      /* bs->rbuffer.count = 0 ... done in refill */
	      buf += r;
	      residue -= r;
	      status = refill (bs);
	      /* Did we reach the end.  */
	      if (status != 0 || bs->rbuffer.count == 0)
		{
		  /* We have something in the buffer return the error on the
		     next call .  */
		  if (count != residue)
		    status = 0;
		  if (pnread)
		    *pnread = count - residue;
		  done = 1;
		}
	    }
	  if (!done)
	    {
	      memcpy(buf, bs->rbuffer.ptr, residue);
	      bs->rbuffer.count -= residue;
	      bs->rbuffer.ptr += residue;
	      if (pnread)
		*pnread = count;
	    }
	}
      monitor_unlock (bs->lock);
      monitor_cleanup_pop (0);
    }
  return status;
}

/*
 * Read at most n-1 characters.
 * Stop when a newline has been read, or the count runs out.
 */
static int
_bs_readline (stream_t stream, char *buf, size_t count, size_t *pnread)
{
  int status = 0;
  struct _bs *bs = (struct _bs *)stream;

  /* Noop.  */
  if (count == 0)
    {
      if (pnread)
	*pnread = 0;
    }
  else if (bs->rbuffer.bufsize == 0)
    {
      /* Use the provided readline.  */
      status = stream_readline (bs->stream, buf, count, pnread);
    }
  else /* Buffered.  */
    {
      char *s = buf;
      char *p, *nl;
      size_t len;
      size_t total = 0;

      monitor_lock (bs->lock);
      monitor_cleanup_push (_bs_cleanup, bs);

      count--;  /* Leave space for the null.  */

      while (count != 0)
	{
	  /* If the buffer is empty refill it.  */
	  len = bs->rbuffer.count;
	  if (len <= 0)
	    {
	      status = refill (bs);
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
	      memcpy (s, p, len);
	      total += len;
	      s[len] = '\0';
	      if (pnread)
		*pnread = total;
	      break;
	    }
	  bs->rbuffer.count -= len;
	  bs->rbuffer.ptr += len;
	  memcpy(s, p, len);
	  total += len;
	  s += len;
	  count -= len;
        }
      *s = 0;
      if (pnread)
	*pnread = s - buf;

      monitor_unlock (bs->lock);
      monitor_cleanup_pop (0);
    }
  return status;
}

static int
_bs_write (stream_t stream, const char *buf, size_t count, size_t *pnwrite)
{
  struct _bs *bs = (struct _bs *)stream;
  int err = 0;
  size_t nwriten = 0;
  size_t total = 0;
  int nleft = count;

  /* First try to send it all.  */
  while (nleft > 0)
    {
      err = stream_write (bs->stream, buf, nleft, &nwriten);
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

static int
_bs_get_fd (stream_t stream, int *pfd)
{
  struct _bs *bs = (struct _bs *)stream;
  return stream_get_fd (bs->stream, pfd);
}

static int
_bs_get_flags (stream_t stream, int *pfl)
{
  struct _bs *bs = (struct _bs *)stream;
  return stream_get_flags (bs->stream, pfl);
}

static int
_bs_get_size (stream_t stream, off_t *psize)
{
  struct _bs *bs = (struct _bs *)stream;
  return stream_get_size (bs->stream, psize);
}

static int
_bs_truncate (stream_t stream, off_t len)
{
  struct _bs *bs = (struct _bs *)stream;
  return stream_truncate (bs->stream, len);
}

static int
_bs_flush (stream_t stream)
{
  struct _bs *bs = (struct _bs *)stream;
  return stream_flush (bs->stream);
}

static int
_bs_get_state (stream_t stream, enum stream_state *pstate)
{
  struct _bs *bs = (struct _bs *)stream;
  return stream_get_state (bs->stream, pstate);
}

static int
_bs_seek (stream_t stream, off_t off, enum stream_whence whence)
{
  struct _bs *bs = (struct _bs *)stream;
  return stream_seek (bs->stream, off, whence);
}

static int
_bs_tell (stream_t stream, off_t *off)
{
  struct _bs *bs = (struct _bs *)stream;
  return stream_tell (bs->stream, off);
}

static struct _stream_vtable _bs_vtable =
{
  _bs_add_ref,
  _bs_release,
  _bs_destroy,

  _bs_open,
  _bs_close,

  _bs_read,
  _bs_readline,
  _bs_write,

  _bs_seek,
  _bs_tell,

  _bs_get_size,
  _bs_truncate,
  _bs_flush,

  _bs_get_fd,
  _bs_get_flags,
  _bs_get_state,
};

int
stream_buffer_create (stream_t *pstream, stream_t stream, size_t bufsize)
{
  struct _bs *bs;

  if (pstream == NULL || stream == NULL || bufsize == 0)
    return MU_ERROR_INVALID_PARAMETER;

  bs = calloc (1, sizeof *bs);
  if (bs == NULL)
    return MU_ERROR_NO_MEMORY;

  bs->base.vtable = &_bs_vtable;
  bs->ref = 1;
  bs->stream = stream;
  bs->rbuffer.bufsize = bufsize;
  monitor_create (&(bs->lock));
  *pstream = &bs->base;
  return 0;
}
