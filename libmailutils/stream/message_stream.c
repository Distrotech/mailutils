/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002, 2005, 2006, 2007, 2009, 2010
   Free Software Foundation, Inc.

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

/* This file implements an MH draftfile stream: a read-only stream used
   to transparently pass MH draftfiles to mailers. The only difference
   between the usual RFC822 and MH draft is that the latter allows to use
   a string of dashes to separate the headers from the body. */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include <mailutils/types.h>
#include <mailutils/address.h>
#include <mailutils/alloc.h>
#include <mailutils/envelope.h>
#include <mailutils/message.h>
#include <mailutils/header.h>
#include <mailutils/body.h>
#include <mailutils/stream.h>
#include <mailutils/util.h>
#include <mailutils/errno.h>
#include <mailutils/error.h>
#include <mailutils/cctype.h>
#include <mailutils/cstr.h>
#include <mailutils/sys/message_stream.h>


static int
_env_msg_date (mu_envelope_t envelope, char *buf, size_t len, size_t *pnwrite)
{
  struct _mu_message_stream *str = mu_envelope_get_owner (envelope);
  
  if (!str || !str->date)
    return EINVAL;
  if (buf)
    {
      strncpy (buf, str->date, len);
      buf[len-1] = 0;
      if (pnwrite)
	*pnwrite = len;
    }
  else if (!pnwrite)
    return EINVAL;
  else
    *pnwrite = strlen (str->date);
  return 0;
}

static int
_env_msg_sender (mu_envelope_t envelope, char *buf, size_t len,
		 size_t *pnwrite)
{
  struct _mu_message_stream *str = mu_envelope_get_owner (envelope);
  
  if (!str || !str->from)
    return EINVAL;
  if (buf)
    {
      strncpy (buf, str->from, len);
      buf[len-1] = 0;
      if (pnwrite)
	*pnwrite = len;
    }
  else if (!pnwrite)
    return EINVAL;
  else
    *pnwrite = strlen (str->from);
    
  return 0;
}


static int
_message_read (mu_stream_t stream, char *optr, size_t osize, size_t *nbytes)
{
  int rc;
  struct _mu_message_stream *s = (struct _mu_message_stream*) stream;
  mu_off_t offset = s->offset + s->envelope_length;
  size_t rsize;
  
  if (offset < s->mark_offset)
    {
      if (offset + osize >= s->mark_offset)
	osize = s->mark_offset - offset;
    }
  else
    offset += s->mark_length;
  /* FIXME: Seeking each time before read is awkward. The streamref
     should be modified to take care of it */
  rc = mu_stream_seek (s->transport, offset, MU_SEEK_SET, NULL);
  if (rc == 0)
    rc = mu_stream_read (s->transport, optr, osize, &rsize);
  if (rc == 0)
    {
      s->offset += rsize;
      if (nbytes)
	*nbytes = rsize;
    }
  else
    s->stream.last_err = rc;
  return rc;
}
  
static int
_message_size (mu_stream_t stream, mu_off_t *psize)
{
  struct _mu_message_stream *s = (struct _mu_message_stream*) stream;
  int rc = mu_stream_size (s->transport, psize);
  
  if (rc == 0)
    *psize -= s->envelope_length + s->mark_length;
  return rc;
}
  
static int
scan_stream (struct _mu_message_stream *str)
{
  char *from = NULL;
  char *env_from = NULL;
  char *env_date = NULL;
  int rc;
  char *buffer = NULL;
  size_t bufsize = 0;
  size_t len;
  mu_off_t body_start, body_end;
  mu_stream_t stream = str->transport;

  if (str->envelope)
    {
      char *s = str->envelope + 5;
      char *p = strchr (s, ' ');
      size_t len;

      if (p)
	{
	  len = p - s;
	  env_from = mu_alloc (len + 1);
	  if (!env_from)
	    return ENOMEM;
	  memcpy (env_from, s, len);
	  env_from[len] = 0;
	  env_date = mu_strdup (p + 1);
	  if (!env_date)
	    {
	      free (env_from);
	      return ENOMEM;
	    }
	}
    }

  rc = mu_stream_seek (stream, 0, MU_SEEK_SET, NULL);
  if (rc)
    return rc;
  while ((rc = mu_stream_getline (stream, &buffer, &bufsize, &len)) == 0
	 && len > 0)
    {
      if (buffer[0] == '\n')
	break;

      if (!env_from || !env_date)
	{
      	  if (!from && mu_c_strncasecmp (buffer, MU_HEADER_FROM,
				         sizeof (MU_HEADER_FROM) - 1) == 0)
	    
	    from = mu_strdup (mu_str_skip_class (buffer +
						 sizeof (MU_HEADER_FROM),
						 MU_CTYPE_SPACE));
	  else if (!env_from
		   && mu_c_strncasecmp (buffer, MU_HEADER_ENV_SENDER,
				        sizeof (MU_HEADER_ENV_SENDER) - 1) == 0)
	    env_from = mu_strdup (mu_str_skip_class (buffer +
					sizeof (MU_HEADER_ENV_SENDER),
						     MU_CTYPE_SPACE));
	  else if (!env_date
		   && mu_c_strncasecmp (buffer, MU_HEADER_ENV_DATE,
				        sizeof (MU_HEADER_ENV_DATE) - 1) == 0)
	    env_date = mu_strdup (mu_str_skip_class (buffer +
						  sizeof (MU_HEADER_ENV_DATE),
						     MU_CTYPE_SPACE));
	}
    }

  free (buffer);

  rc = mu_stream_seek (stream, 0, MU_SEEK_CUR, &body_start);
  if (rc)
    return rc;
  rc = mu_stream_size (stream, &body_end);
  if (rc)
    return rc;
  
  if (!env_from)
    {
      if (from)
	{
	  mu_address_t addr;
	  
	  mu_address_create (&addr, from);
	  if (!addr
	      || mu_address_aget_email (addr, 1, &env_from))
	    env_from = mu_strdup ("GNU-Mailutils");
	  mu_address_destroy (&addr);
	}
      else
	env_from = mu_strdup ("GNU-MH");
    }
  free (from);
  
  if (!env_date)
    {
      struct tm *tm;
      time_t t;
      char date[80]; /* FIXME: This size is way too big */

      time(&t);
      tm = gmtime(&t);
      mu_strftime (date, sizeof (date), "%a %b %e %H:%M:%S %Y", tm);
      env_date = strdup (date);
    }

  str->from = env_from;
  str->date = env_date;

  str->body_start = body_start;
  str->body_end = body_end - 1;
  
  return 0;
}

static int
_message_open (mu_stream_t stream)
{
  struct _mu_message_stream *s = (struct _mu_message_stream*) stream;
  size_t offset, len;
  char *buffer = NULL;
  size_t bufsize = 0;
  int rc;

  offset = 0;
  mu_stream_seek (s->transport, 0, MU_SEEK_SET, NULL);
  while ((rc = mu_stream_getline (s->transport, &buffer, &bufsize,
				  &len)) == 0
	 && len > 0)
    {
      if (offset == 0 && memcmp (buffer, "From ", 5) == 0)
	{
	  s->envelope_length = len;
	  s->envelope = mu_strdup (buffer);
	  if (!s->envelope)
	    return ENOMEM;
	  s->envelope[len - 1] = 0;
	}
      else if (mu_mh_delim (buffer))
	{
	  s->mark_offset = offset;
	  s->mark_length = len - 1; /* do not count the terminating newline */
	  break;
	}

      offset += len;
    }
  free (buffer);

  return scan_stream (s);
}

static int
_message_close (mu_stream_t stream)
{
  struct _mu_message_stream *s = (struct _mu_message_stream*) stream;
  return s->stream.last_err = mu_stream_close (s->transport);
}

static void
_message_done (mu_stream_t stream)
{
  struct _mu_message_stream *s = (struct _mu_message_stream*) stream;

  free (s->envelope);
  free (s->date);
  free (s->from);
  mu_stream_destroy (&s->transport);
}

static int
_message_seek (struct _mu_stream *stream, mu_off_t off, mu_off_t *presult)
{ 
  struct _mu_message_stream *s = (struct _mu_message_stream*) stream;
  mu_off_t size;

  mu_stream_size (stream, &size);
  if (off < 0 || off >= size)
    return ESPIPE;
  s->offset = off;
  *presult = off;
  return 0;
}

const char *
_message_error_string (struct _mu_stream *stream, int rc)
{
  struct _mu_message_stream *str = (struct _mu_message_stream*) stream;
  return mu_stream_strerror (str->transport, rc);
}

int
mu_message_stream_create (mu_stream_t *pstream, mu_stream_t src, int flags)
{
  struct _mu_message_stream *s;
  int sflag;
  int rc;
  mu_stream_t stream;
  
  mu_stream_get_flags (src, &sflag);
  sflag &= MU_STREAM_SEEK;
  
  if (!flags)
    flags = MU_STREAM_READ;
  if (flags & (MU_STREAM_WRITE|MU_STREAM_CREAT|MU_STREAM_APPEND))
    return EINVAL;
  s = (struct _mu_message_stream *) _mu_stream_create (sizeof (*s),
						       flags | sflag);
  if (!s)
    return ENOMEM;

  rc = mu_streamref_create (&s->transport, src);
  if (rc)
    {
      free (s);
      return rc;
    }
  s->stream.open = _message_open;
  s->stream.close = _message_close;
  s->stream.done = _message_done;
  s->stream.read = _message_read;
  s->stream.size = _message_size;
  s->stream.seek = _message_seek;
  s->stream.error_string = _message_error_string;

  stream = (mu_stream_t)s;
  rc = mu_stream_open (stream);
  if (rc)
    mu_stream_destroy (&stream);
  else
    *pstream = stream;
  return rc;
}


/* *************************** MH draft message **************************** */



static int
_body_obj_size (mu_body_t body, size_t *size)
{
  mu_message_t msg = mu_body_get_owner (body);
  struct _mu_message_stream *str = mu_message_get_owner (msg);

  if (size)
    *size = str->body_end - str->body_start + 1;
  return 0;
}



int
mu_stream_to_message (mu_stream_t instream, mu_message_t *pmsg)
{
  mu_envelope_t env;
  mu_message_t msg;
  mu_body_t body;
  mu_stream_t bstream;
  mu_stream_t draftstream;
  int rc;
  struct _mu_message_stream *sp;
  
  /* FIXME: Perhaps MU_STREAM_NO_CLOSE is needed */
  if ((rc = mu_message_stream_create (&draftstream, instream, 0)))
    return rc;

  if ((rc = mu_message_create (&msg, draftstream)))
    {
      mu_stream_destroy (&draftstream);
      return rc;
    }
  
  mu_message_set_stream (msg, draftstream, draftstream);
  
  if ((rc = mu_envelope_create (&env, draftstream)))
    {
      mu_message_destroy (&msg, draftstream);
      mu_stream_destroy (&draftstream);
      return rc;
    }
  
  mu_envelope_set_date (env, _env_msg_date, draftstream);
  mu_envelope_set_sender (env, _env_msg_sender, draftstream);
  mu_message_set_envelope (msg, env, draftstream);

  mu_body_create (&body, msg);
  /* FIXME: It would be cleaner to use ioctl here */
  sp = (struct _mu_message_stream *) draftstream;
  rc = mu_streamref_create_abridged (&bstream, instream,
				     sp->body_start, sp->body_end);
  if (rc)
    {
      mu_body_destroy (&body, msg);
      mu_message_destroy (&msg, draftstream);
      mu_stream_destroy (&draftstream);
      return rc;
    }
  
  mu_body_set_stream (body, bstream, msg);
  mu_body_set_size (body, _body_obj_size, msg);
  mu_message_set_body (msg, body, draftstream);

  *pmsg = msg;
  return 0;
}