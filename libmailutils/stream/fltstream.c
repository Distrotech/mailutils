/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2009, 2010 Free Software Foundation, Inc.

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
#include <mailutils/types.h>
#include <mailutils/alloc.h>
#include <mailutils/error.h>
#include <mailutils/errno.h>
#include <mailutils/nls.h>
#include <mailutils/stream.h>
#include <mailutils/sys/filter.h>

#define MFB_BASE(buf) ((buf).base)
#define MFB_CURPTR(buf) ((buf).base + (buf).pos)
#define MFB_ENDPTR(buf) ((buf).base + (buf).level)

#define MFB_SIZE(buf) ((buf).size)
#define MFB_LEVEL(buf) ((buf).level)
#define MFB_POS(buf) ((buf).pos)
#define MFB_RDBYTES(buf) \
  (MFB_LEVEL (buf) - MFB_POS (buf))
#define MFB_FREESIZE(buf) \
  (MFB_SIZE (buf) - MFB_LEVEL (buf))

#define MBF_CLEAR(buf) ((buf).pos = (buf).level = 0)
#define MBF_FREE(buf) free ((buf).base)
  
static void
init_iobuf (struct mu_filter_io *io, struct _mu_filter_stream *fs)
{
  io->input = MFB_CURPTR (fs->inbuf);
  io->isize = MFB_RDBYTES (fs->inbuf);
  io->output = MFB_ENDPTR (fs->outbuf);
  io->osize = MFB_FREESIZE (fs->outbuf);
  io->errcode = 0;
  io->eof = 0;
}

static int
filter_stream_init (struct _mu_filter_stream *fs)
{
  if (fs->xdata)
    {
      struct mu_filter_io iobuf;
      memset (&iobuf, 0, sizeof (iobuf));
      if (fs->xcode (fs->xdata, mu_filter_init, &iobuf) == mu_filter_falure)
	return iobuf.errcode;
    }
  return 0;
}

static int
MFB_require (struct _mu_filter_buffer *buf, size_t size)
{
  if (size > MFB_FREESIZE (*buf))
    {
      /* Compact the buffer */
      if (MFB_POS (*buf))
	{
	  memmove (MFB_BASE (*buf), MFB_CURPTR (*buf), MFB_RDBYTES (*buf));
	  buf->level -= buf->pos;
	  buf->pos = 0;
	}
      if (size > MFB_FREESIZE (*buf))
	{
	  char *p;

	  size += MFB_LEVEL (*buf);
	  p = realloc (buf->base, size);
	  if (!p)
	    return ENOMEM;
	  buf->size = size;
	  buf->base = p;
	}
    }
  return 0;
}

static void
MFB_advance_pos (struct _mu_filter_buffer *buf, size_t delta)
{
  buf->pos += delta;
  if (buf->pos == buf->level)
    buf->pos = buf->level = 0;
}

static void
MFB_advance_level (struct _mu_filter_buffer *buf, size_t delta)
{
  buf->level += delta;
}

static int
filter_read (mu_stream_t stream, char *buf, size_t size, size_t *pret)
{
  struct _mu_filter_stream *fs = (struct _mu_filter_stream *)stream;
  struct mu_filter_io iobuf;
  size_t min_input_level = MU_FILTER_BUF_SIZE;
  size_t min_output_size = MU_FILTER_BUF_SIZE;
  enum mu_filter_command cmd = mu_filter_xcode;
  size_t total = 0;
  int stop = 0;
  int again = 0;
  
  do
    {
      size_t rdsize;

      if (MFB_RDBYTES (fs->outbuf) == 0)
	{
	  enum mu_filter_result res;
	  int rc;
	  
	  if (MFB_RDBYTES (fs->inbuf) < min_input_level && !again)
	    {
	      rc = MFB_require (&fs->inbuf, min_input_level);
	      if (rc)
		return rc;
	      rc = mu_stream_read (fs->transport,
				   MFB_ENDPTR (fs->inbuf),
				   MFB_FREESIZE (fs->inbuf),
				   &rdsize);
	      if (rc)
		return rc;
	      if (rdsize == 0 &&
		  MFB_RDBYTES (fs->outbuf) == 0 &&
		  MFB_RDBYTES (fs->inbuf) == 0)
		cmd = mu_filter_lastbuf;
	      
	      MFB_advance_level (&fs->inbuf, rdsize);
	    }

	  if (min_output_size < MFB_RDBYTES (fs->inbuf))
	    min_output_size = MFB_RDBYTES (fs->inbuf);
	  rc = MFB_require (&fs->outbuf, min_output_size);
	  if (rc)
	    return rc;
      
	  init_iobuf (&iobuf, fs);

	  if (cmd != mu_filter_lastbuf)
	    cmd = mu_stream_eof (fs->transport) ?
	      mu_filter_lastbuf : mu_filter_xcode;
	  res = fs->xcode (fs->xdata, cmd, &iobuf);
	  switch (res)
	    {
	    case mu_filter_again:
	      if (++again > MU_FILTER_MAX_AGAIN)
		{
		  /* FIXME: What filter? Need some id. */
		  mu_error (_("filter returned `again' too many times"));
		  again = 0;
		}
	      break;
	      
	    case mu_filter_ok:
	      again = 0;
	      if (cmd == mu_filter_lastbuf || iobuf.eof)
		{
		  _mu_stream_seteof (stream);
		  stop = 1;
		}
	      break;
	  
	    case mu_filter_falure:
	      return iobuf.errcode;
	      
	    case mu_filter_moreinput:
	      min_input_level = iobuf.isize;
	      continue;
	      
	    case mu_filter_moreoutput:
	      min_output_size = iobuf.osize;
	      continue;
	    }
      
	  if (iobuf.isize > MFB_RDBYTES (fs->inbuf)
	      || iobuf.osize > MFB_FREESIZE (fs->outbuf))
	    return MU_ERR_FAILURE; /* FIXME: special error code? */
	  
	  /* iobuf.osize contains number of bytes written to output */
	  MFB_advance_level (&fs->outbuf, iobuf.osize);
	  
	  /* iobuf.isize contains number of bytes read from input */
	  MFB_advance_pos (&fs->inbuf, iobuf.isize);
	}

      rdsize = size - total;
      if (rdsize > MFB_RDBYTES (fs->outbuf))
	rdsize = MFB_RDBYTES (fs->outbuf);
      memcpy (buf + total, MFB_CURPTR (fs->outbuf), rdsize);
      MFB_advance_pos (&fs->outbuf, rdsize);
      total += rdsize;

    }
  while (!stop && (total < size || again));
  
  *pret = total;
  return 0;
}

static int
filter_rd_flush (mu_stream_t stream)
{
  struct _mu_filter_stream *fs = (struct _mu_filter_stream *)stream;
  return filter_stream_init (fs);
}

static int
filter_write_internal (mu_stream_t stream, enum mu_filter_command cmd,
		       const char *buf, size_t size, size_t *pret)
{
  struct _mu_filter_stream *fs = (struct _mu_filter_stream *)stream;
  struct mu_filter_io iobuf;
  size_t min_input_level = cmd == mu_filter_xcode ? MU_FILTER_BUF_SIZE : 0;
  size_t min_output_size = MU_FILTER_BUF_SIZE;
  size_t total = 0;
  int rc = 0;
  int again;
  int stop = 0;
  
  do
    {
      size_t rdsize;
      enum mu_filter_result res;

      if (MFB_RDBYTES (fs->inbuf) < min_input_level)
	{
	  rdsize = size - total;
	  if (rdsize == 0)
	    break;
	  rc = MFB_require (&fs->inbuf, min_input_level);
	  if (rc)
	    break;
	  if (rdsize > MFB_FREESIZE (fs->inbuf))
	    rdsize = MFB_FREESIZE (fs->inbuf);
	  memcpy (MFB_ENDPTR (fs->inbuf), buf + total, rdsize);
	  MFB_advance_level (&fs->inbuf, rdsize);
	  total += rdsize;
	}

      if (min_output_size < MFB_RDBYTES (fs->inbuf))
	min_output_size = MFB_RDBYTES (fs->inbuf);
      rc = MFB_require (&fs->outbuf, min_output_size);
      if (rc)
	return rc;
      
      init_iobuf (&iobuf, fs);

      res = fs->xcode (fs->xdata, cmd, &iobuf);
      switch (res)
	{
	case mu_filter_again:
	  if (++again > MU_FILTER_MAX_AGAIN)
	    {
	      /* FIXME: What filter? Need some id. */
	      mu_error (_("filter returned `again' too many times"));
	      again = 0;
	    }
	  break;

	case mu_filter_ok:
	  again = 0;
	  if (cmd == mu_filter_lastbuf || iobuf.eof)
	    {
	      _mu_stream_seteof (stream);
	      stop = 1;
	    }
	  break;
	  
	case mu_filter_falure:
	  return iobuf.errcode;
	  
	case mu_filter_moreinput:
	  min_input_level = iobuf.isize;
	  continue;
	  
	case mu_filter_moreoutput:
	  min_output_size = iobuf.osize;
	  continue;
	}
      
      if (iobuf.isize > MFB_RDBYTES (fs->inbuf)
	  || iobuf.osize > MFB_FREESIZE (fs->outbuf))
	return MU_ERR_FAILURE; /* FIXME: special error code? */
      
      /* iobuf.osize contains number of bytes written to output */
      MFB_advance_level (&fs->outbuf, iobuf.osize);
      
      /* iobuf.isize contains number of bytes read from input */
      MFB_advance_pos (&fs->inbuf, iobuf.isize);
      
      rc = mu_stream_write (fs->transport,
			    MFB_CURPTR (fs->outbuf),
			    MFB_RDBYTES (fs->outbuf),
			    &rdsize);
      if (rc == 0)
	MFB_advance_pos (&fs->outbuf, rdsize);
      else
	break;
    }
  while (!stop && (MFB_RDBYTES (fs->outbuf) || again));
  if (pret)
    *pret = total;
  else if (total < size && rc == 0)
    rc = MU_ERR_FAILURE;
  return rc;
}

static int
filter_write (mu_stream_t stream, const char *buf, size_t size, size_t *pret)
{
  return filter_write_internal (stream, mu_filter_xcode, buf, size, pret);
}

static int
filter_wr_flush (mu_stream_t stream)
{
  struct _mu_filter_stream *fs = (struct _mu_filter_stream *)stream;
  size_t dummy;
  int rc = filter_write_internal (stream, mu_filter_flush, NULL, 0, &dummy);
  if (rc == 0)
    rc = mu_stream_flush (fs->transport);
  return rc;
}

static int
filter_seek (struct _mu_stream *stream, mu_off_t off, mu_off_t *ppos)
{
  struct _mu_filter_stream *fs = (struct _mu_filter_stream *)stream;
  int status;

  status = mu_stream_seek (fs->transport, 0, MU_SEEK_SET, NULL);
  if (status)
    return status;
  stream->offset = 0;
  return mu_stream_skip_input_bytes (stream, off, ppos);
}

static int
filter_ctl (struct _mu_stream *stream, int op, void *ptr)
{
  struct _mu_filter_stream *fs = (struct _mu_filter_stream *)stream;
  mu_transport_t *ptrans;

  switch (op)
    {
    case MU_IOCTL_GET_TRANSPORT:
      if (!ptr)
	return EINVAL;
      ptrans = ptr;
      ptrans[0] = (mu_transport_t) fs->transport;
      ptrans[1] = NULL;
      break;

    default:
      return mu_stream_ioctl (fs->transport, op, ptr);
    }
  return 0;
}

static const char *
filter_error_string (struct _mu_stream *stream, int rc)
{
  struct _mu_filter_stream *fs = (struct _mu_filter_stream *)stream;
  const char *p = mu_stream_strerror (fs->transport, rc);
  if (!p)
    p = mu_strerror (rc);
  return p;
}

static void
filter_done (mu_stream_t stream)
{
  struct _mu_filter_stream *fs = (struct _mu_filter_stream *)stream;
  MBF_FREE (fs->inbuf);
  MBF_FREE (fs->outbuf);
  if (fs->xdata)
    {
      fs->xcode (fs->xdata, mu_filter_done, NULL);
      free (fs->xdata);
    }
  mu_stream_destroy (&fs->transport);
}

static int
filter_wr_close (mu_stream_t stream)
{
  struct _mu_filter_stream *fs = (struct _mu_filter_stream *)stream;
  if (!mu_stream_eof (stream))
    {
      size_t dummy;
      int rc = filter_write_internal (stream, mu_filter_lastbuf, NULL, 0,
				      &dummy);
      if (rc)
	return rc;
    }
  MBF_CLEAR (fs->inbuf);
  MBF_CLEAR (fs->outbuf);
  return mu_stream_close (fs->transport);
}

static int
filter_rd_close (mu_stream_t stream)
{
  struct _mu_filter_stream *fs = (struct _mu_filter_stream *)stream;
  MBF_CLEAR (fs->inbuf);
  MBF_CLEAR (fs->outbuf);
  return mu_stream_close (fs->transport);
}


static int
filter_read_through (struct _mu_stream *stream,
		     char *buf, size_t bufsize,
		     size_t *pnread)
{
  struct _mu_filter_stream *fs = (struct _mu_filter_stream *)stream;
  return mu_stream_read (fs->transport, buf, bufsize, pnread);
}

static int
filter_write_through (struct _mu_stream *stream,
		      const char *buf, size_t bufsize,
		      size_t *pnwrite)
{
  struct _mu_filter_stream *fs = (struct _mu_filter_stream *)stream;
  return mu_stream_write (fs->transport, buf, bufsize, pnwrite);
}

static int
filter_wait (struct _mu_stream *stream, int *pflags, struct timeval *tvp)
{
  struct _mu_filter_stream *fs = (struct _mu_filter_stream *)stream;
  /* FIXME: Take into account internal buffer state. */
  return mu_stream_wait (fs->transport, pflags, tvp);
}


int
mu_filter_stream_create (mu_stream_t *pflt,
			 mu_stream_t str,
			 int mode, 
			 mu_filter_xcode_t xcode,
			 void *xdata, int flags)
{
  int rc;
  struct _mu_filter_stream *fs;

  if ((flags & MU_STREAM_RDWR) == MU_STREAM_RDWR
      || !(flags & MU_STREAM_RDWR)
      || (flags & (MU_STREAM_WRITE|MU_STREAM_SEEK)) ==
          (MU_STREAM_WRITE|MU_STREAM_SEEK)
      || (flags & (MU_STREAM_RDTHRU|MU_STREAM_WRTHRU)) ==
	  (MU_STREAM_RDTHRU|MU_STREAM_WRTHRU)
      || (flags & (MU_STREAM_READ|MU_STREAM_RDTHRU)) ==
	  (MU_STREAM_READ|MU_STREAM_RDTHRU)
      || (flags & (MU_STREAM_WRITE|MU_STREAM_WRTHRU)) ==
          (MU_STREAM_WRITE|MU_STREAM_WRTHRU))
    return EINVAL;
 
  fs = (struct _mu_filter_stream *) _mu_stream_create (sizeof (*fs), flags);
  if (!fs)
    return ENOMEM;
  
  flags |= _MU_STR_OPEN;
  if (flags & MU_STREAM_READ)
    {
      fs->stream.read = filter_read;
      fs->stream.flush = filter_rd_flush;
      fs->stream.close = filter_rd_close;
      if (flags & MU_STREAM_WRTHRU)
	{
	  flags |= MU_STREAM_WRITE;
	  fs->stream.write = filter_write_through;
	}
    }
  else
    {
      fs->stream.write = filter_write;
      fs->stream.flush = filter_wr_flush;
      fs->stream.close = filter_wr_close;
      if (flags & MU_STREAM_RDTHRU)
	{
	  flags |= MU_STREAM_READ;
	  fs->stream.read = filter_read_through;
	}
    }
  fs->stream.done = filter_done;
  if (flags & MU_STREAM_SEEK)
    fs->stream.seek = filter_seek;
  fs->stream.ctl = filter_ctl;
  fs->stream.wait = filter_wait;
  fs->stream.error_string = filter_error_string;
  fs->stream.flags = flags;

  if (!(flags & MU_STREAM_AUTOCLOSE))
    mu_stream_ref (str);
  fs->transport = str;
  fs->xcode = xcode;
  fs->xdata = xdata;
  fs->mode = mode;
  
  mu_stream_set_buffer ((mu_stream_t) fs, mu_buffer_full, MU_FILTER_BUF_SIZE);

  rc = filter_stream_init (fs);
  if (rc)
    {
      free (fs);
      return rc;
    }
    
  *pflt = (mu_stream_t) fs;
  return 0;
}


