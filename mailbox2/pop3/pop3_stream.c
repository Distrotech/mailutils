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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <string.h>
#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif

#include <stdlib.h>
#include <mailutils/sys/pop3.h>

int
pop3_set_stream (pop3_t pop3, stream_t stream)
{
  /* Sanity checks.  */
  if (pop3 == NULL)
    return MU_ERROR_INVALID_PARAMETER;

  /* If the stream was set before we will leak.  But do what they say.  */
  pop3->stream = stream;
  return 0;
}

int
pop3_get_stream (pop3_t pop3, stream_t *pstream)
{
  /* Sanity checks.  */
  if (pop3 == NULL || pstream == NULL)
    return MU_ERROR_INVALID_PARAMETER;

  if (pop3->stream == NULL)
    {
      int status = stream_tcp_create (&(pop3->stream));
      if (status)
	return status;
      /* Using the awkward stream_t buffering.  */
      /* stream_setbufsiz (pop3->stream, 1024); */
    }
  *pstream = pop3->stream;
  return 0;
}

/* Implementation of the stream for TOP and RETR.  */
static int p_add_ref       __P ((stream_t));
static int p_release       __P ((stream_t));
static int p_destroy       __P ((stream_t));

static int p_open          __P ((stream_t, const char *, int, int));
static int p_close         __P ((stream_t));

static int p_read          __P ((stream_t, char *, size_t, size_t *));
static int p_readline      __P ((stream_t, char *, size_t, size_t *));
static int p_write         __P ((stream_t, const char *, size_t, size_t *));

static int p_seek          __P ((stream_t, off_t, enum stream_whence));
static int p_tell          __P ((stream_t, off_t *));

static int p_get_size      __P ((stream_t, off_t *));
static int p_truncate      __P ((stream_t, off_t));
static int p_flush         __P ((stream_t));

static int p_get_fd        __P ((stream_t, int *));
static int p_get_flags     __P ((stream_t, int *));
static int p_get_state     __P ((stream_t, enum stream_state *));

static struct _stream_vtable p_s_vtable =
{
  p_add_ref,
  p_release,
  p_destroy,

  p_open,
  p_close,

  p_read,
  p_readline,
  p_write,

  p_seek,
  p_tell,

  p_get_size,
  p_truncate,
  p_flush,

  p_get_fd,
  p_get_flags,
  p_get_state,
};

int
pop3_stream_create (pop3_t pop3, stream_t *pstream)
{
  struct p_stream *p_stream;

  p_stream = malloc (sizeof *p_stream);
  if (p_stream == NULL)
    return MU_ERROR_NO_MEMORY;

  p_stream->base.vtable = &p_s_vtable;
  p_stream->ref = 1;
  p_stream->done = 0;
  p_stream->pop3 = pop3;
  *pstream = &p_stream->base;
  return 0;
}

static int
p_add_ref (stream_t stream)
{
  struct p_stream *p_stream = (struct p_stream *)stream;
  return ++p_stream->ref;
}

static int
p_release (stream_t stream)
{
  struct p_stream *p_stream = (struct p_stream *)stream;
  if (--p_stream->ref == 0)
    {
      p_destroy (stream);
      return 0;
    }
  return p_stream->ref;
}

static int
p_destroy (stream_t stream)
{
  struct p_stream *p_stream = (struct p_stream *)stream;
  if (!p_stream->done)
    {
      char buf[128];
      size_t n = 0;
      while (pop3_readline (p_stream->pop3, buf, sizeof buf, &n) > 0
             && n > 0)
        n = 0;
    }
  p_stream->pop3->state = POP3_NO_STATE;
  free (stream);
  return 0;
}

static int
p_open (stream_t stream, const char *h, int p, int f)
{
  struct p_stream *p_stream = (struct p_stream *)stream;
  (void)f;
  return pop3_connect (p_stream->pop3, h, p);
}

static int
p_close (stream_t stream)
{
  struct p_stream *p_stream = (struct p_stream *)stream;
  return pop3_disconnect (p_stream->pop3);
}

static int
p_read (stream_t stream, char *buf, size_t buflen, size_t *pn)
{
  struct p_stream *p_stream = (struct p_stream *)stream;
  size_t n = 0;
  int status = 0;
  if (!p_stream->done)
    {
      do
	{
	  size_t nread = 0;

	  /* The pop3_readline () function will always read one less to
	     be able to terminate the buffer, this will cause serious grief
	     for stream_read() where it is legitimate to have a buffer of
	     1 char.  So we must catch here or change the behavoiour of
	     XXX_readline.  */
	  if (buflen == 1)
	    {
	      char buffer[2];
	      *buffer = '\0';
	      status = pop3_readline (p_stream->pop3, buffer, 2, &nread);
	      *buf = *buffer;
	    }
	  else
	    status = pop3_readline (p_stream->pop3, buf, buflen, &nread);

	  if (status != 0)
	    break;
	  if (nread == 0)
	    {
	      p_stream->pop3->state = POP3_NO_STATE;
	      p_stream->done = 1;
	      break;
	    }
	  n += nread;
	  buflen -= nread;
	  buf += nread;
	}
      while (buflen > 0);
    }
  if (pn)
    *pn = n;
  return status;
}

static int
p_readline (stream_t stream, char *buf, size_t buflen, size_t *pn)
{
  struct p_stream *p_stream = (struct p_stream *)stream;
  size_t n = 0;
  int status = 0;
  if (!p_stream->done)
    {
      status = pop3_readline (p_stream->pop3, buf, buflen, &n);
      if (n == 0)
	{
	  p_stream->pop3->state = POP3_NO_STATE;
	  p_stream->done = 1;
	}
    }
  if (pn)
    *pn = n;
  return status;
}

static int
p_write (stream_t stream, const char *buf, size_t buflen, size_t *pn)
{
  struct p_stream *p_stream = (struct p_stream *)stream;
  return stream_write (p_stream->pop3->stream, buf, buflen, pn);
}

static int
p_seek (stream_t stream, off_t offset, enum stream_whence whence)
{
  struct p_stream *p_stream = (struct p_stream *)stream;
  return stream_seek (p_stream->pop3->stream, offset, whence);
}

static int
p_tell (stream_t stream, off_t *offset)
{
  struct p_stream *p_stream = (struct p_stream *)stream;
  return stream_tell (p_stream->pop3->stream, offset);
}

static int
p_get_size (stream_t stream, off_t *size)
{
  struct p_stream *p_stream = (struct p_stream *)stream;
  return stream_get_size (p_stream->pop3->stream, size);
}

static int
p_truncate (stream_t stream, off_t size)
{
  struct p_stream *p_stream = (struct p_stream *)stream;
  return stream_truncate (p_stream->pop3->stream, size);
}

static int
p_flush (stream_t stream)
{
  struct p_stream *p_stream = (struct p_stream *)stream;
  return stream_flush (p_stream->pop3->stream);
}

static int
p_get_fd (stream_t stream, int *fd)
{
  struct p_stream *p_stream = (struct p_stream *)stream;
  return stream_get_fd (p_stream->pop3->stream, fd);
}

static int
p_get_flags (stream_t stream, int *flags)
{
  struct p_stream *p_stream = (struct p_stream *)stream;
  return stream_get_flags (p_stream->pop3->stream, flags);
}

static int
p_get_state (stream_t stream, enum stream_state *state)
{
  struct p_stream *p_stream = (struct p_stream *)stream;
  return stream_get_state (p_stream->pop3->stream, state);
}
