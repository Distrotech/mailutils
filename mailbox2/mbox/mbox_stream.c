/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Library Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with GNU Mailutils; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>

#include <mailutils/error.h>
#include <mailutils/refcount.h>
#include <mailutils/sys/stream.h>
#include <mailutils/sys/mbox.h>

#undef min
#define min(a,b) ((a) < (b) ? (a) : (b))

struct _stream_mbox
{
  struct _stream base;
  mu_refcount_t refcount;
  mbox_t mbox;
  unsigned int msgno;
  int is_header;
};

static int
_stream_mbox_ref (stream_t stream)
{
  struct _stream_mbox *ms = (struct _stream_mbox *)stream;
  return mu_refcount_inc (ms->refcount);
}

static void
_stream_mbox_destroy (stream_t *pstream)
{
  struct _stream_mbox *ms = (struct _stream_mbox *)*pstream;
  if (mu_refcount_dec (ms->refcount) == 0)
    {
      mu_refcount_destroy (&ms->refcount);
      if (ms->mbox && ms->msgno && ms->msgno <= ms->mbox->messages_count)
	{
	  /* Loose the reference only if it is the same that we saved
	     on the mailbox.  */
	  if (ms->is_header && ms == (struct _stream_mbox *)
	      (ms->mbox->umessages[ms->msgno - 1]->header.stream))
	    {
	      ms->mbox->umessages[ms->msgno - 1]->header.stream = NULL;
	    }
	  else if (ms == (struct _stream_mbox *)
		   (ms->mbox->umessages[ms->msgno - 1]->body.stream))
	    {
	      ms->mbox->umessages[ms->msgno - 1]->body.stream = NULL;
	    }
	}
      free (ms);
    }
}

static int
_stream_mbox_open (stream_t stream, const char *name, int port, int flags)
{
  (void)stream; (void)name; (void)port; (void)flags;
  return 0;
}

static int
_stream_mbox_close (stream_t stream)
{
  (void)stream;
  return 0;
}

static int
_stream_mbox_read0 (stream_t carrier, void *buf, size_t buflen, off_t off,
		    size_t *pn, off_t start, off_t end)
{
  off_t ln = end - (start + off);
  int status = 0;
  if (ln > 0)
    {
      size_t n = min ((size_t)ln, buflen);
      status = stream_read (carrier, buf, n, start + off, pn);
    }
  return status;
}

static int
_stream_mbox_read (stream_t stream, void *buf, size_t buflen, off_t off,
		   size_t *pnread)
{
  int status = 0;
  struct _stream_mbox *ms = (struct _stream_mbox *)stream;
  size_t n = 0;

  if (buf && buflen)
    {
      if (ms->mbox && ms->msgno && ms->msgno <= ms->mbox->messages_count)
	{
	  mbox_message_t mum = ms->mbox->umessages[ms->msgno - 1];
	  if (ms->is_header)
	    status = _stream_mbox_read0 (ms->mbox->carrier, buf, buflen, off,
					 &n, mum->header.start,
					 mum->header.end);
	  else
	    status = _stream_mbox_read0 (ms->mbox->carrier, buf, buflen, off,
					 &n, mum->header.start,
					 mum->header.end);
	}
    }

  if (pnread)
    *pnread = n;
  return status;
}

static int
_stream_mbox_readline0 (stream_t carrier, char *buf, size_t buflen, off_t off,
			size_t *pn, off_t start, off_t end)
{
  off_t ln = end - (start + off);
  int status = 0;
  if (ln > 0)
    {
      size_t n = min ((size_t)ln, buflen);
      status = stream_readline (carrier, buf, n, start + off, pn);
    }
  return status;
}

/*
 * Read at most n-1 characters.
 * Stop when a newline has been read, or the count runs out.
 */
static int
_stream_mbox_readline (stream_t stream, char *buf, size_t buflen, off_t off,
		       size_t *pnread)
{
  int status = 0;
  struct _stream_mbox *ms = (struct _stream_mbox *)stream;
  size_t nread = 0;

  if (buf && buflen)
    {
      if (ms->mbox && ms->msgno && ms->msgno <= ms->mbox->messages_count)
	{
	  mbox_message_t mum = ms->mbox->umessages[ms->msgno - 1];
	  if (ms->is_header)
	    status = _stream_mbox_readline0 (ms->mbox->carrier, buf, buflen,
					     off, &nread, mum->header.start,
					     mum->header.end);
	  else
	    status = _stream_mbox_readline0 (ms->mbox->carrier, buf, buflen,
					     off, &nread, mum->body.start,
					     mum->body.end);
	}
    }

  if (pnread)
    *pnread = nread;
  return status;
}

static int
_stream_mbox_write (stream_t stream, const void *buf, size_t buflen, off_t off,
		    size_t *pnwrite)
{
  (void)stream; (void)buf; (void)buflen; (void)off, (void)pnwrite;
  return MU_ERROR_IO;
}

static int
_stream_mbox_get_fd (stream_t stream, int *pfd)
{
  struct _stream_mbox *ms = (struct _stream_mbox *)stream;
  if (ms->mbox)
    return stream_get_fd (ms->mbox->carrier, pfd);
  return MU_ERROR_IO;
}

static int
_stream_mbox_get_flags (stream_t stream, int *pfl)
{
  struct _stream_mbox *ms = (struct _stream_mbox *)stream;
  if (ms->mbox)
    return stream_get_flags (ms->mbox->carrier, pfl);
  return MU_ERROR_IO;
}

static int
_stream_mbox_get_size (stream_t stream, off_t *psize)
{
  struct _stream_mbox *ms = (struct _stream_mbox *)stream;

  if (psize)
    {
      if (ms->mbox && ms->msgno && ms->msgno <= ms->mbox->messages_count)
	{
	  if (ms->is_header)
	    *psize = ms->mbox->umessages[ms->msgno - 1]->header.end
	      - ms->mbox->umessages[ms->msgno - 1]->header.start;
	  else
	    *psize = ms->mbox->umessages[ms->msgno - 1]->body.end
	      - ms->mbox->umessages[ms->msgno - 1]->body.start;
	}
      else
	*psize = 0;
    }
  return 0;
}

static int
_stream_mbox_truncate (stream_t stream, off_t len)
{
  (void)stream; (void)len;
  return MU_ERROR_IO;
}

static int
_stream_mbox_flush (stream_t stream)
{
  (void)stream;
  return 0;
}

static int
_stream_mbox_get_state (stream_t stream, enum stream_state *pstate)
{
  struct _stream_mbox *ms = (struct _stream_mbox *)stream;
  if (ms->mbox)
    return stream_get_state (ms->mbox->carrier, pstate);
  return MU_ERROR_IO;
}

static int
_stream_mbox_is_seekable (stream_t stream)
{
  struct _stream_mbox *ms = (struct _stream_mbox *)stream;
  if (ms->mbox)
    return stream_is_seekable (ms->mbox->carrier);
  return MU_ERROR_IO;
}

static int
_stream_mbox_tell (stream_t stream, off_t *off)
{
  struct _stream_mbox *ms = (struct _stream_mbox *)stream;
  if (ms->mbox)
    return stream_tell (ms->mbox->carrier, off);
  return MU_ERROR_IO;
}

static int
_stream_mbox_is_readready (stream_t stream, int timeout)
{
  struct _stream_mbox *ms = (struct _stream_mbox *)stream;
  if (ms->mbox)
    return stream_is_readready (ms->mbox->carrier, timeout);
  return MU_ERROR_IO;
}

static int
_stream_mbox_is_writeready (stream_t stream, int timeout)
{
  (void)stream; (void)timeout;
  return 0;
}

static int
_stream_mbox_is_exceptionpending (stream_t stream, int timeout)
{
  (void)stream; (void)timeout;
  return 0;
}

static int
_stream_mbox_is_open (stream_t stream)
{
  struct _stream_mbox *ms = (struct _stream_mbox *)stream;
  if (ms->mbox)
    return stream_is_open (ms->mbox->carrier);
  return MU_ERROR_IO;
}


static struct _stream_vtable _stream_mbox_vtable =
{
  _stream_mbox_ref,
  _stream_mbox_destroy,

  _stream_mbox_open,
  _stream_mbox_close,

  _stream_mbox_read,
  _stream_mbox_readline,
  _stream_mbox_write,

  _stream_mbox_tell,

  _stream_mbox_get_size,
  _stream_mbox_truncate,
  _stream_mbox_flush,

  _stream_mbox_get_fd,
  _stream_mbox_get_flags,
  _stream_mbox_get_state,

  _stream_mbox_is_seekable,
  _stream_mbox_is_readready,
  _stream_mbox_is_writeready,
  _stream_mbox_is_exceptionpending,

  _stream_mbox_is_open
};

static int
_stream_mbox_ctor (struct _stream_mbox *ms, mbox_t mbox, unsigned int msgno,
		   int is_header)
{
  mu_refcount_create (&ms->refcount);
  if (ms->refcount == NULL)
    return MU_ERROR_NO_MEMORY;

  ms->mbox = mbox;
  ms->msgno = msgno;
  ms->is_header = is_header;
  ms->base.vtable = &_stream_mbox_vtable;
  return 0;
}

void
_stream_mbox_dtor (stream_t stream)
{
  struct _stream_mbox *ms = (struct _stream_mbox *)stream;
  mu_refcount_destroy (&ms->refcount);
  if (ms->mbox && ms->msgno && ms->msgno <= ms->mbox->messages_count)
    {
      /* Loose the reference only if it is the same that we saved
	 on the mailbox.  */
      if (ms->is_header && ms == (struct _stream_mbox *)
	  (ms->mbox->umessages[ms->msgno - 1]->header.stream))
	{
	  ms->mbox->umessages[ms->msgno - 1]->header.stream = NULL;
	}
      else if (ms == (struct _stream_mbox *)
	       (ms->mbox->umessages[ms->msgno - 1]->body.stream))
	{
	  ms->mbox->umessages[ms->msgno - 1]->body.stream = NULL;
	}
    }
  ms->mbox = NULL;
  ms->msgno = 0;
  ms->is_header = 0;
}

int
stream_mbox_set_msgno (stream_t stream, unsigned int msgno)
{
  struct _stream_mbox *ms = (struct _stream_mbox *)stream;
  if (ms)
    ms->msgno = msgno;
  return 0;
}

int
stream_mbox_create (stream_t *pstream, mbox_t mbox, unsigned int msgno,
		    int is_header)
{
  struct _stream_mbox *ms;
  int status;

  if (pstream == NULL || mbox == NULL || msgno == 0)
    return MU_ERROR_INVALID_PARAMETER;

  ms = calloc (1, sizeof *ms);
  if (ms == NULL)
    return MU_ERROR_NO_MEMORY;

  status = _stream_mbox_ctor (ms, mbox, msgno, is_header);
  if (status != 0)
    return status;
  *pstream = &ms->base;
  return 0;
}
