/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA  */

/* This file implements an MH draftfile stream: a read-only stream used
   to transparently pass MH draftfiles to the mailers. The only difference
   between the usual RFC822 and MH draft is that the latter allows to use
   a string of dashes to separate the headers from the body. */

#include <mh.h>
#include <mailutils/stream.h>

struct _mhdraft_stream {
  stream_t stream;     /* Actual stream */
  size_t mark_offset;  /* Offset of the header separator */
  size_t mark_length;  /* Length of the header separator (not counting the
			  newline) */
};

static int
_mhdraft_read (stream_t stream, char *optr, size_t osize,
	       off_t offset, size_t *nbytes)
{
  struct _mhdraft_stream *s = stream_get_owner (stream);

  if (offset < s->mark_offset)
    {
      if (offset + osize >= s->mark_offset)
	osize = s->mark_offset - offset;
    }
  else
    offset += s->mark_length;
  return stream_read (s->stream, optr, osize, offset, nbytes);
}
  
static int
_mhdraft_readline (stream_t stream, char *optr, size_t osize,
		   off_t offset, size_t *nbytes)
{
  struct _mhdraft_stream *s = stream_get_owner (stream);
    
  if (offset < s->mark_offset)
    {
      if (offset + osize >= s->mark_offset)
	{
	  int rc;
	  size_t n;
	  size_t rdsize = s->mark_offset - offset;
	  rc = stream_readline (s->stream, optr, rdsize, offset, &n);
	  if (rc == 0)
	    {
	      if (nbytes)
		*nbytes = n;
	    }
	  return rc;
	}
    }
  else
    offset += s->mark_length;

  return stream_readline (s->stream, optr, osize, offset, nbytes);
}
  
static int
_mhdraft_size (stream_t stream, off_t *psize)
{
  struct _mhdraft_stream *s = stream_get_owner (stream);
  int rc = stream_size (s->stream, psize);
  
  if (rc == 0)
    *psize -= s->mark_length;
  return rc;
}
  
static int
_mhdraft_open (stream_t stream)
{
  struct _mhdraft_stream *s = stream_get_owner (stream);
  size_t offset, len;
  char buffer[256];
  int rc;

  offset = 0;
  while ((rc = stream_readline (s->stream, buffer, sizeof buffer,
				offset, &len)) == 0
	 && len > 0)
    {
      if (_mh_delim (buffer))
	{
	  s->mark_offset = offset;
	  s->mark_length = len - 1; /* do not count the terminating newline */
	  break;
	}

      offset += len;
    }
  return 0;
}

static int
_mhdraft_close (stream_t stream)
{
  struct _mhdraft_stream *s = stream_get_owner (stream);
  return stream_close (s->stream);
}

static void
_mhdraft_destroy (stream_t stream)
{
  struct _mhdraft_stream *s = stream_get_owner (stream);
  if (s->stream)
    stream_destroy (&s->stream, stream_get_owner (s->stream));
  free (s);
}
    
int
mhdraft_stream_create (stream_t *stream, stream_t src, int flags)
{
  struct _mhdraft_stream *s;
  int rc;

  if (!flags)
    flags = MU_STREAM_READ;
  if (flags != MU_STREAM_READ)
    return EINVAL;

  s = calloc (1, sizeof (*s));
  if (s == NULL)
    return ENOMEM;

  s->stream = src;
  
  rc = stream_create (stream, flags|MU_STREAM_NO_CHECK, s);
  if (rc)
    {
      free (s);
      return rc;
    }
  
  stream_set_open (*stream, _mhdraft_open, s);
  stream_set_close (*stream, _mhdraft_close, s);
  stream_set_destroy (*stream, _mhdraft_destroy, s);
  stream_set_readline (*stream, _mhdraft_readline, s);
  stream_set_read (*stream, _mhdraft_read, s);
  stream_set_size (*stream, _mhdraft_size, s);
  return 0;  
}


