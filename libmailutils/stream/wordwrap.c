/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2016 Free Software Foundation, Inc.

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
#include <mailutils/types.h>
#include <mailutils/stream.h>
#include <mailutils/sys/stream.h>
#include <mailutils/alloc.h>
#include <mailutils/error.h>
#include <mailutils/errno.h>
#include <mailutils/nls.h>
#include <mailutils/cctype.h>

struct mu_wordwrap_stream
{
  struct _mu_stream stream;
  unsigned left_margin;
  unsigned right_margin;
  char *buffer;
  unsigned offset;
  mu_stream_t transport;
};

static int
is_word (int c)
{
  return !mu_isspace (c);
}

static int
full_write (struct mu_wordwrap_stream *str, size_t length)
{
  size_t n, rdsize;
  for (n = 0; n < length; )
    {
      int rc = mu_stream_write (str->transport,
				str->buffer + n, length - n,
				&rdsize);
      if (rc)
	return rc;
      n += rdsize;
    }
  return 0;
}

static int
_wordwrap_flush_line (struct mu_wordwrap_stream *str, int lookahead)
{
  size_t length, word_start, word_len = 0;
  int nl = 0;
  char savech;
  int rc;
  
  length = word_start = str->offset;
  if (str->offset > 0 && lookahead)
    {
      if (is_word (str->buffer[str->offset - 1]) && is_word (lookahead))
	{
	  /* Find the nearest word boundary */
	  for (length = str->offset; length > str->left_margin; length--)
	    {
	      if (!is_word (str->buffer[length - 1]))
		break;
	    }
	  if (length == str->left_margin)
	    {
	      rc = full_write (str, str->offset);
	      if (rc == 0)
		str->offset = 0;
	      return rc;
	    }
	  word_start = length;
	}
    }

  while (length > 0 && mu_isspace (str->buffer[length - 1]))
    length--;
  
  if (length == 0 || str->buffer[length - 1] != '\n')
    {
      savech = str->buffer[length];
      str->buffer[length] = '\n';
      nl = 1;
    }
  else
    nl = 0;
  
  /* Flush the line buffer content */
  rc = full_write (str, length + nl);
  if (rc)
    /* FIXME: Inconsistent state after error */
    return rc;

  if (nl)
    str->buffer[length] = savech;
  
  /* Adjust the buffer */
  memset (str->buffer, ' ', str->left_margin);
  if (word_start > str->left_margin)
    {
      word_len = str->offset - word_start;
      if (word_len)
	memmove (str->buffer + str->left_margin, str->buffer + word_start,
		 word_len);
    }
  str->offset = str->left_margin + word_len;
  return 0;
}
  
static int
_wordwrap_write (mu_stream_t stream, const char *iptr, size_t isize,
		 size_t *nbytes)
{
  struct mu_wordwrap_stream *str = (struct mu_wordwrap_stream *) stream;
  size_t n;

  for (n = 0; n < isize; n++)
    {
      if (str->offset == str->right_margin
	  || (str->offset > str->left_margin
	      && str->buffer[str->offset - 1] == '\n'))
	_wordwrap_flush_line (str, iptr[n]);
      if (str->offset == str->left_margin && mu_isblank (iptr[n]))
	continue;
      str->buffer[str->offset++] = iptr[n];
    }
  if (nbytes)
    *nbytes = isize;
  return 0;
}

static int
_wordwrap_flush (mu_stream_t stream)
{
  struct mu_wordwrap_stream *str = (struct mu_wordwrap_stream *)stream;
  if (str->offset > str->left_margin)
    _wordwrap_flush_line (str, 0);
  return mu_stream_flush (str->transport);
}

static void
_wordwrap_done (mu_stream_t stream)
{
  struct mu_wordwrap_stream *str = (struct mu_wordwrap_stream *)stream;
  mu_stream_destroy (&str->transport);
}

static int
_wordwrap_close (mu_stream_t stream)
{
  struct mu_wordwrap_stream *str = (struct mu_wordwrap_stream *)stream;
  return mu_stream_close (str->transport);
}

static int
set_margin (mu_stream_t stream, unsigned lmargin, int off)
{
  struct mu_wordwrap_stream *str = (struct mu_wordwrap_stream *)stream;

  if (off < 0 && -off > str->left_margin)
    return EINVAL;
  lmargin += off;
  
  if (lmargin >= str->right_margin)
    return EINVAL;

  /* Flush the stream if the new margin is set to the left of the current
     column, or it equals the current column and the character in previous
     column is a word constituent, or the last character on line is a newline.
  */
  if (str->offset > str->left_margin
      && (lmargin < str->offset
	  || (lmargin == str->offset && is_word (str->buffer[str->offset - 1]))
	  || str->buffer[str->offset - 1] == '\n'))
    _wordwrap_flush (stream);

  if (lmargin > str->offset)
    memset (str->buffer + str->offset, ' ', lmargin - str->offset);
  str->left_margin = lmargin;
  str->offset = lmargin;

  return 0;
}

static int
_wordwrap_ctl (mu_stream_t stream, int code, int opcode, void *arg)
{
  struct mu_wordwrap_stream *str = (struct mu_wordwrap_stream *)stream;
  int status;

  switch (code)
    {
    case MU_IOCTL_WORDWRAPSTREAM:
      switch (opcode)
	{
	case MU_IOCTL_WORDWRAP_GET_MARGIN:
	  /* Get left margin */
	  if (!arg)
	    return MU_ERR_OUT_PTR_NULL;
	  *(unsigned *)arg = str->left_margin;
	  break;
	    
	case MU_IOCTL_WORDWRAP_SET_MARGIN:
	  /* Set left margin */
	  if (!arg)
	    return EINVAL;
	  else
	    return set_margin (stream, *(unsigned*)arg, 0);

	case MU_IOCTL_WORDWRAP_SET_NEXT_MARGIN:
	  if (!arg)
	    return EINVAL;
	  else
	    {
	      unsigned marg = *(unsigned*)arg;
	      if (marg >= str->right_margin)
		return EINVAL;
	      str->left_margin = marg;
	    }
	  break;
	  
	case MU_IOCTL_WORDWRAP_MOVE_MARGIN:
	  if (!arg)
	    return EINVAL;
	  else
	    return set_margin (stream, str->offset, *(int*)arg);

	case MU_IOCTL_WORDWRAP_GET_COLUMN:
	  if (!arg)
	    return EINVAL;
	  *(unsigned*)arg = str->offset;
	  break;

	default:
	  return EINVAL;
	}
      break;
      
    case MU_IOCTL_TRANSPORT:
      if (!arg)
	return EINVAL;
      else
	{
	  mu_transport_t *ptrans = arg;
	  switch (opcode)
	    {
	    case MU_IOCTL_OP_GET:
	      ptrans[0] = (mu_transport_t) str->transport;
	      ptrans[1] = NULL;
	      break;

	    case MU_IOCTL_OP_SET:
	      ptrans = arg;
	      if (ptrans[0])
		str->transport = (mu_stream_t) ptrans[0];
	      break;

	    default:
	      return EINVAL;
	    }
	}
      break;

    case MU_IOCTL_SUBSTREAM:
      if (str->transport &&
          ((status = mu_stream_ioctl (str->transport, code, opcode, arg)) == 0 ||
           status != ENOSYS))
        return status;
      /* fall through */

    case MU_IOCTL_TOPSTREAM:
      if (!arg)
	return EINVAL;
      else
	{
	  mu_stream_t *pstr = arg;
	  switch (opcode)
	    {
	    case MU_IOCTL_OP_GET:
	      pstr[0] = str->transport;
	      mu_stream_ref (pstr[0]);
	      pstr[1] = NULL;
	      break;

	    case MU_IOCTL_OP_SET:
	      mu_stream_unref (str->transport);
	      str->transport = pstr[0];
	      mu_stream_ref (str->transport);
	      break;

	    default:
	      return EINVAL;
	    }
	}
      break;

    case MU_IOCTL_FILTER:
      return mu_stream_ioctl (str->transport, code, opcode, arg);
      
    default:
      return ENOSYS;
    }
  return 0;
}
  
int
mu_wordwrap_stream_create (mu_stream_t *pstream, mu_stream_t transport,
			   size_t left_margin, size_t right_margin)
{
  int rc;
  mu_stream_t stream;
  struct mu_wordwrap_stream *str;

  if (right_margin == 0 || left_margin >= right_margin)
    return EINVAL;
  
  str = (struct mu_wordwrap_stream *)
             _mu_stream_create (sizeof (*str), MU_STREAM_APPEND);
  if (!str)
    return ENOMEM;
  str->stream.close = _wordwrap_close;
  str->stream.write = _wordwrap_write;
  //  str->stream.size = _wordwrap_size;
  str->stream.done = _wordwrap_done;
  str->stream.flush = _wordwrap_flush;
  str->stream.ctl = _wordwrap_ctl;
  
  str->transport = transport;
  mu_stream_ref (transport);
  str->left_margin = left_margin;
  str->right_margin = right_margin;
  str->buffer = mu_alloc (str->right_margin+1);
  memset (str->buffer, ' ', str->left_margin);
  str->offset = str->left_margin;
  
  stream = (mu_stream_t) str;
  rc = mu_stream_open (stream);
  if (rc)
    mu_stream_destroy (&stream);
  else
    *pstream = stream;
  return rc;
}
