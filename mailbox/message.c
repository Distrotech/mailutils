/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002, 2004, 2005, 2006, 2007, 2009,
   2010 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library; if not, write to the
   Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301 USA */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <pwd.h>

#include <mailutils/cctype.h>
#include <mailutils/address.h>
#include <mailutils/attribute.h>
#include <mailutils/auth.h>
#include <mailutils/body.h>
#include <mailutils/debug.h>
#include <mailutils/envelope.h>
#include <mailutils/errno.h>
#include <mailutils/folder.h>
#include <mailutils/header.h>
#include <mailutils/mailbox.h>
#include <mailutils/mutil.h>
#include <mailutils/observer.h>
#include <mailutils/stream.h>
#include <mailutils/mu_auth.h>
#include <mailutils/nls.h>
#include <mailutils/md5.h>
#include <mailutils/io.h>

#include <mailutils/sys/message.h>
#include <mailutils/sys/stream.h>

#define MESSAGE_MODIFIED        0x10000
#define MESSAGE_INTERNAL_STREAM 0x20000


/* Message stream */

enum _message_stream_state
  {
    _mss_init,
    _mss_header,
    _mss_body,
    _mss_eof
  };

struct _mu_message_stream
{
  struct _mu_stream stream;
  mu_message_t msg;
  enum _message_stream_state state;
  mu_stream_t transport;
  mu_off_t limit;
};

static int
_check_stream_state (struct _mu_message_stream *str)
{
  int rc = 0;
  
  if (str->transport && mu_stream_eof (str->transport))
    mu_stream_destroy (&str->transport);
  
  switch (str->state)
    {
    case _mss_init:
      if (!str->transport)
	{
	  rc = mu_header_get_streamref (str->msg->header, &str->transport);
	  if (rc == 0)
	    {
	      str->state = _mss_header;
	      rc = mu_stream_seek (str->transport, 0, MU_SEEK_SET, NULL);
	    }
	}
      break;
      
    case _mss_header:
      if (!str->transport)
	{
	  rc = mu_body_get_streamref (str->msg->body, &str->transport);
	  if (rc == 0)
	    {
	      str->state = _mss_body;
	      rc = mu_stream_seek (str->transport, 0, MU_SEEK_SET, NULL);
	    }
	}
      break;
      
    case _mss_body:
      if (!str->transport)
	str->state = _mss_eof;
    case _mss_eof:
      break;
    }
  return rc;
}

static void
_message_stream_done (struct _mu_stream *str)
{
  struct _mu_message_stream *sp = (struct _mu_message_stream *)str;
  mu_stream_destroy (&sp->transport);
}

static int
_message_stream_flush (struct _mu_stream *str)
{
  struct _mu_message_stream *sp = (struct _mu_message_stream *)str;
  int rc = _check_stream_state (sp);
  if (rc)
    return rc;
  return mu_stream_flush (sp->transport);
}
  
static int
_message_stream_size (struct _mu_stream *str, mu_off_t *psize)
{
  struct _mu_message_stream *sp = (struct _mu_message_stream *)str;
  size_t hsize, bsize;
  mu_header_size (sp->msg->header, &hsize);
  mu_body_size (sp->msg->body, &bsize);
  if (psize)
    *psize = hsize + bsize;
  return 0;
}

static int
_message_stream_seek (struct _mu_stream *str, mu_off_t off, mu_off_t *ppos)
{
  struct _mu_message_stream *sp = (struct _mu_message_stream *)str;
  size_t hsize, size;
  int rc;
  
  rc = _check_stream_state (sp);
  if (rc)
    return rc;
  mu_header_size (sp->msg->header, &hsize);
  mu_body_size (sp->msg->body, &size);
  size += hsize;
  
  if (off < 0 || off >= size)
    return ESPIPE;

  switch (sp->state)
    {
    case _mss_eof:
      sp->state = _mss_init;
      rc = _check_stream_state (sp);
      if (rc)
	return rc;
      /* fall through */
    case _mss_header:
      if (off < hsize)
	break;
      mu_stream_destroy (&sp->transport);
      rc = _check_stream_state (sp);
      if (rc)
	return rc;
      /* fall through */
    case _mss_body:
      off -= hsize;
      break;

    default:
      break;
    }
  rc = mu_stream_seek (sp->transport, off, MU_SEEK_SET, &off);
  if (rc == 0)
    {
      if (sp->state == _mss_body)
	off += hsize;
      *ppos = off;
    }
  return rc;
}

static int
_message_stream_read (struct _mu_stream *str, char *buf, size_t bufsize,
		      size_t *pnread)
{
  struct _mu_message_stream *sp = (struct _mu_message_stream *)str;
  size_t nread = 0;
  int rc;
  
  while (bufsize)
    {
      size_t n;
      rc = _check_stream_state (sp);
      if (rc)
	break;
      if (sp->state == _mss_eof)
	break;
      rc = mu_stream_read (sp->transport, buf, bufsize, &n);
      nread += n;
      buf += n;
      bufsize -= n;
    }
  *pnread = nread;
  return rc;
}

static int
_message_stream_readdelim (struct _mu_stream *str, char *buf, size_t bufsize,
			   int delim, size_t *pnread)
{
  struct _mu_message_stream *sp = (struct _mu_message_stream *)str;
  size_t nread = 0;
  int rc;
  
  while (bufsize)
    {
      size_t n;
      rc = _check_stream_state (sp);
      if (rc)
	break;
      if (sp->state == _mss_eof)
	break;
      rc = mu_stream_readdelim (sp->transport, buf, bufsize, delim, &n);
      if (rc || n == 0)
	break;
      nread += n;
      buf += n;
      bufsize -= n;
    }
  *pnread = nread;
  return rc;
}  

#if 0
static int
_message_stream_write (struct _mu_stream *str,
		       const char *buf, size_t bufsize,
		       size_t *pnwritten)
{
  struct _mu_message_stream *sp = (struct _mu_message_stream *)str;
  
  /* FIXME */
}
#endif

static int
_message_stream_create (mu_stream_t *pmsg, mu_message_t msg, int flags)
{
  struct _mu_message_stream *sp;

  sp = (struct _mu_message_stream *) _mu_stream_create (sizeof (*sp),
							flags | MU_STREAM_SEEK);
  if (!sp)
    return ENOMEM;

  sp->stream.read = _message_stream_read;
  sp->stream.readdelim = _message_stream_readdelim;
  /* FIXME: Write is not defined */
  /*  sp->stream.write = _message_stream_write;*/
  sp->stream.done = _message_stream_done;
  sp->stream.flush = _message_stream_flush;
  sp->stream.seek = _message_stream_seek; 
  sp->stream.size = _message_stream_size;
  sp->state = _mss_init;
  sp->msg = msg;
  *pmsg = (mu_stream_t) sp;
  return 0;
}


enum eoh_state
  {
    eoh_no,
    eoh_maybe,
    eoh_yes
  };

/* Message header stuff */
static enum eoh_state
string_find_eoh (enum eoh_state eoh, const char *str, size_t len,
		 size_t *ppos)
{
  size_t pos;

  if (eoh == eoh_maybe && *str == '\n')
    {
      *ppos = 0;
      return eoh_yes;
    }
  
  for (pos = 0; pos < len - 1; pos++)
    if (str[pos] == '\n' && str[pos + 1] == '\n')
      {
	*ppos = pos + 1;
	return eoh_yes;
      }
  
  *ppos = pos + 1;
  return str[pos] == '\n' ? eoh_maybe : eoh_no;
}

#define MIN_HEADER_BUF_SIZE 2048

static int
_header_fill (mu_stream_t stream, char **pbuf, size_t *plen)
{
  int status = 0;
  char *buffer = NULL;
  size_t bufsize = 0;
  char inbuf[MIN_HEADER_BUF_SIZE];
  size_t nread;
  enum eoh_state eoh = eoh_no;
  
  status = mu_stream_seek (stream, 0, MU_SEEK_SET, NULL);
  if (status)
    return status;
      
  while (eoh != eoh_yes
	 && (status = mu_stream_read (stream, inbuf, sizeof (inbuf), &nread))
	    == 0
	 && nread)
    {
      char *nbuf;
      size_t len;

      eoh = string_find_eoh (eoh, inbuf, nread, &len);
      
      nbuf = realloc (buffer, bufsize + len);
      if (!nbuf)
	{
	  status = ENOMEM;
	  break;
	}
      memcpy (nbuf + bufsize, inbuf, len);
      buffer = nbuf;
      bufsize += len;
    }

  if (status)
    free (buffer);
  else
    {
      *pbuf = buffer;
      *plen = bufsize;
    }
  return status;
}
    
static int
message_header_fill (void *data, char **pbuf, size_t *plen)
{
  int status = 0;
  mu_message_t msg = data;
  mu_stream_t stream;

  status = mu_message_get_streamref (msg, &stream);
  if (status == 0)
    {
      status = _header_fill (stream, pbuf, plen);
      mu_stream_destroy (&stream);
    }
  return status;
}


/* Message envelope */
static int
message_envelope_date (mu_envelope_t envelope, char *buf, size_t len,
		       size_t *pnwrite)
{
  mu_message_t msg = mu_envelope_get_owner (envelope);
  time_t t;
  size_t n;

  if (msg == NULL)
    return EINVAL;

  /* FIXME: extract the time from "Date:".  */

  if (buf == NULL || len == 0)
    {
      n = MU_ENVELOPE_DATE_LENGTH;
    }
  else
    {
      char tmpbuf[MU_ENVELOPE_DATE_LENGTH+1];
      t = time (NULL);
      n = mu_strftime (tmpbuf, sizeof tmpbuf, 
                       MU_ENVELOPE_DATE_FORMAT, localtime (&t));
      n = mu_cpystr (buf, tmpbuf, len);
    }
  if (pnwrite)
    *pnwrite = n;
  return 0;
}

static int
message_envelope_sender (mu_envelope_t envelope, char *buf, size_t len,
			 size_t *pnwrite)
{
  mu_message_t msg = mu_envelope_get_owner (envelope);
  mu_header_t header = NULL;
  size_t n = 0;
  int status;

  if (msg == NULL)
    return EINVAL;

  /* Can it be extracted from the From:  */
  mu_message_get_header (msg, &header);
  status = mu_header_get_value (header, MU_HEADER_FROM, NULL, 0, &n);
  if (status == 0 && n != 0)
    {
      char *sender;
      mu_address_t address = NULL;
      sender = calloc (1, n + 1);
      if (sender == NULL)
	return ENOMEM;
      mu_header_get_value (header, MU_HEADER_FROM, sender, n + 1, NULL);
      if (mu_address_create (&address, sender) == 0)
	mu_address_get_email (address, 1, buf, n + 1, pnwrite);
      free (sender);
      mu_address_destroy (&address);
      return 0;
    }
  else if (status == EAGAIN)
    return status;

  /* oops! We are still here */
  {
    struct mu_auth_data *auth = mu_get_auth_by_uid (getuid ());
    const char *sender = auth ? auth->name : "unknown";
    n = strlen (sender);
    if (buf && len > 0)
      {
	len--; /* One for the null.  */
	n = (n < len) ? n : len;
	memcpy (buf, auth->name, n);
	buf[n] = '\0';
      }
    if (auth)
      mu_auth_data_free (auth);
  }

  if (pnwrite)
    *pnwrite = n;
  return 0;
}



/*  Allocate ressources for the mu_message_t.  */
int
mu_message_create (mu_message_t *pmsg, void *owner)
{
  mu_message_t msg;
  int status;

  if (pmsg == NULL)
    return MU_ERR_OUT_PTR_NULL;
  msg = calloc (1, sizeof (*msg));
  if (msg == NULL)
    return ENOMEM;
  status = mu_monitor_create (&msg->monitor, 0, msg);
  if (status != 0)
    {
      free (msg);
      return status;
    }
  msg->owner = owner;
  msg->ref = 1;
  *pmsg = msg;
  return 0;
}

void
mu_message_destroy (mu_message_t *pmsg, void *owner)
{
  if (pmsg && *pmsg)
    {
      mu_message_t msg = *pmsg;
      mu_monitor_t monitor = msg->monitor;
      int destroy_lock = 0;

      mu_monitor_wrlock (monitor);
      /* Note: msg->ref may be incremented by mu_message_ref without
	 additional checking for its owner, therefore decrementing
	 it must also occur independently of the owner checking. Due
	 to this inconsistency ref may reach negative values, which
	 is very unfortunate.

	 The `owner' stuff is a leftover from older mailutils versions.
	 There is an ongoing attempt to remove it in the stream-cleanup
	 branch. When it is ready, it will be merged to the HEAD and this
	 will finally resolve this issue. */
      if (msg->ref > 0)
	msg->ref--;
      if ((msg->owner && msg->owner == owner)
	  || (msg->owner == NULL && msg->ref <= 0))
	{
	  destroy_lock =  1;
	  /* Notify the listeners.  */
	  /* FIXME: to be removed since we do not support this event.  */
	  if (msg->observable)
	    {
	      mu_observable_notify (msg->observable, MU_EVT_MESSAGE_DESTROY,
				    msg);
	      mu_observable_destroy (&msg->observable, msg);
	    }

	  /* Envelope.  */
	  if (msg->envelope)
	    mu_envelope_destroy (&msg->envelope, msg);

	  /* Header.  */
	  if (msg->header)
	    mu_header_destroy (&msg->header);

	  /* Body.  */
	  if (msg->body)
	    mu_body_destroy (&msg->body, msg);

	  /* Attribute.  */
	  if (msg->attribute)
	    mu_attribute_destroy (&msg->attribute, msg);

	  /* Stream.  */
	  if (msg->stream)
	    mu_stream_destroy (&msg->stream);

	  /*  Mime.  */
	  if (msg->mime)
	    mu_mime_destroy (&msg->mime);

	  /* Loose the owner.  */
	  msg->owner = NULL;

	  free (msg);
	}
      mu_monitor_unlock (monitor);
      if (destroy_lock)
	mu_monitor_destroy (&monitor, msg);
      /* Loose the link */
      *pmsg = NULL;
    }
}

int
mu_message_create_copy (mu_message_t *to, mu_message_t from)
{
  int status = 0;
  mu_stream_t fromstr = NULL;
  mu_stream_t tostr = NULL;
  size_t n = 0;
  char buf[512];

  if (!to)
    return MU_ERR_OUT_PTR_NULL;
  if (!from)
    return EINVAL;

  if ((status = mu_message_create (to, NULL)))
    return status;

  mu_message_get_streamref (from, &fromstr);
  mu_message_get_streamref (*to, &tostr);

  status = mu_stream_seek (fromstr, 0, MU_SEEK_SET, NULL);
  if (status == 0)
    while ((status = mu_stream_readline (fromstr, buf, sizeof (buf), &n)) == 0
	   && n > 0)
      mu_stream_write (tostr, buf, n, NULL);

  mu_stream_destroy (&fromstr);
  mu_stream_destroy (&tostr);
  
  if (status)
    mu_message_destroy (to, NULL);
  
  return status;
}

int
mu_message_ref (mu_message_t msg)
{
  if (msg)
    {
      mu_monitor_wrlock (msg->monitor);
      msg->ref++;
      mu_monitor_unlock (msg->monitor);
    }
  return 0;
}

void *
mu_message_get_owner (mu_message_t msg)
{
  return (msg == NULL) ? NULL : msg->owner;
}

int
mu_message_is_modified (mu_message_t msg)
{
  int mod = 0;
  if (msg)
    {
      mod |= mu_header_is_modified (msg->header);
      mod |= mu_attribute_is_modified (msg->attribute);
      mod |= mu_body_is_modified (msg->body);
      mod |= msg->flags;
    }
  return mod;
}

int
mu_message_clear_modified (mu_message_t msg)
{
  if (msg)
    {
      if (msg->header)
	mu_header_clear_modified (msg->header);
      if (msg->attribute)
	mu_attribute_clear_modified (msg->attribute);
      if (msg->body)
	mu_body_clear_modified (msg->body);
      msg->flags &= ~MESSAGE_MODIFIED;
    }
  return 0;
}

int
mu_message_get_mailbox (mu_message_t msg, mu_mailbox_t *pmailbox)
{
  if (msg == NULL)
    return EINVAL;
  if (pmailbox == NULL)
    return MU_ERR_OUT_PTR_NULL;
  *pmailbox = msg->mailbox;
  return 0;
}

int
mu_message_set_mailbox (mu_message_t msg, mu_mailbox_t mailbox, void *owner)
{
  if (msg == NULL)
    return EINVAL;
  if (msg->owner != owner)
    return EACCES;
  msg->mailbox = mailbox;
  return 0;
}

int
mu_message_get_header (mu_message_t msg, mu_header_t *phdr)
{
  if (msg == NULL)
    return EINVAL;
  if (phdr == NULL)
    return MU_ERR_OUT_PTR_NULL;

  if (msg->header == NULL)
    {
      mu_header_t header;
      int status = mu_header_create (&header, NULL, 0);
      if (status != 0)
	return status;
      if (msg->stream)
	mu_header_set_fill (header, message_header_fill, msg);
      msg->header = header;
    }
  *phdr = msg->header;
  return 0;
}

/* Note: mu_message_set_header steals the reference to hdr */
int
mu_message_set_header (mu_message_t msg, mu_header_t hdr, void *owner)
{
  if (msg == NULL )
    return EINVAL;
  if (msg->owner != owner)
     return EACCES;
  if (msg->header)
    mu_header_destroy (&msg->header);
  msg->header = hdr;
  msg->flags |= MESSAGE_MODIFIED;
  return 0;
}

int
mu_message_get_body (mu_message_t msg, mu_body_t *pbody)
{
  if (msg == NULL)
    return EINVAL;
  if (pbody == NULL)
    return MU_ERR_OUT_PTR_NULL;

  /* Is it a floating mesg.  */
  if (msg->body == NULL)
    {
      mu_body_t body;
      int status = mu_body_create (&body, msg);
      if (status != 0)
	return status;
      /* If a stream is already set use it to create the body stream.  */
      /* FIXME: I'm not sure if the second condition is really needed */
      if (msg->stream && (msg->flags & MESSAGE_INTERNAL_STREAM))
	{
	  size_t size = 0;
	  mu_stream_t stream;
	  int flags = 0;
	  
	  status = mu_header_size (msg->header, &size);
	  if (status)
	    return status;
	  
	  mu_stream_get_flags (msg->stream, &flags);
	  status = mu_streamref_create_abridged (&stream, msg->stream,
						 size, 0);
	  if (status)
	    {
	      mu_body_destroy (&body, msg);
	      return status;
	    }
	  mu_body_set_stream (body, stream, msg);
	}
      msg->body = body;
    }
  *pbody = msg->body;
  return 0;
}

int
mu_message_set_body (mu_message_t msg, mu_body_t body, void *owner)
{
  if (msg == NULL )
    return EINVAL;
  if (msg->owner != owner)
    return EACCES;
  /* Make sure we destroy the old if it was owned by the mesg.  */
  /* FIXME:  I do not know if somebody has already a ref on this ? */
  if (msg->body)
    mu_body_destroy (&msg->body, msg);
  msg->body = body;
  msg->flags |= MESSAGE_MODIFIED;
  return 0;
}

int
mu_message_set_stream (mu_message_t msg, mu_stream_t stream, void *owner)
{
  if (msg == NULL)
    return EINVAL;
  if (msg->owner != owner)
    return EACCES;
  if (msg->stream)
    mu_stream_destroy (&msg->stream);
  msg->stream = stream;
  msg->flags |= MESSAGE_MODIFIED;
  msg->flags &= ~MESSAGE_INTERNAL_STREAM;
  return 0;
}

static int
_message_get_stream (mu_message_t msg, mu_stream_t *pstream, int ref)
{
  if (msg == NULL)
    return EINVAL;
  if (pstream == NULL)
    return MU_ERR_OUT_PTR_NULL;

  if (msg->stream == NULL)
    {
      int status = _message_stream_create (&msg->stream, msg, MU_STREAM_RDWR);
      if (status)
	return status;
      msg->flags |= MESSAGE_INTERNAL_STREAM;
    }

  if (!ref)
    {
      *pstream = msg->stream;
      return 0;
    }
  return mu_streamref_create (pstream, msg->stream);
}

int
mu_message_get_stream (mu_message_t msg, mu_stream_t *pstream)
{
  /* FIXME: Deprecation warning */
  return _message_get_stream (msg, pstream, 0);
}

int
mu_message_get_streamref (mu_message_t msg, mu_stream_t *pstream)
{
  return _message_get_stream (msg, pstream, 1);
}

int
mu_message_set_lines (mu_message_t msg, int (*_lines)
		   (mu_message_t, size_t *), void *owner)
{
  if (msg == NULL)
    return EINVAL;
  if (msg->owner != owner)
    return EACCES;
  msg->_lines = _lines;
  return 0;
}

int
mu_message_lines (mu_message_t msg, size_t *plines)
{
  size_t hlines, blines;
  int ret = 0;

  if (msg == NULL)
    return EINVAL;
  /* Overload.  */
  if (msg->_lines)
    return msg->_lines (msg, plines);
  if (plines)
    {
      hlines = blines = 0;
      if ( ( ret = mu_header_lines (msg->header, &hlines) ) == 0 )
	      ret = mu_body_lines (msg->body, &blines);
      *plines = hlines + blines;
    }
  return ret;
}

int
mu_message_set_size (mu_message_t msg, int (*_size)
		  (mu_message_t, size_t *), void *owner)
{
  if (msg == NULL)
    return EINVAL;
  if (msg->owner != owner)
    return EACCES;
  msg->_size = _size;
  return 0;
}

int
mu_message_size (mu_message_t msg, size_t *psize)
{
  size_t hsize, bsize;
  int ret = 0;

  if (msg == NULL)
    return EINVAL;
  /* Overload ? */
  if (msg->_size)
    return msg->_size (msg, psize);
  if (psize)
    {
      mu_header_t hdr = NULL;
      mu_body_t body = NULL;
      
      hsize = bsize = 0;
      mu_message_get_header (msg, &hdr);
      mu_message_get_body (msg, &body);
      if ( ( ret = mu_header_size (hdr, &hsize) ) == 0 )
	ret = mu_body_size (body, &bsize);
      *psize = hsize + bsize;
    }
  return ret;
}

int
mu_message_get_envelope (mu_message_t msg, mu_envelope_t *penvelope)
{
  if (msg == NULL)
    return EINVAL;
  if (penvelope == NULL)
    return MU_ERR_OUT_PTR_NULL;

  if (msg->envelope == NULL)
    {
      mu_envelope_t envelope;
      int status = mu_envelope_create (&envelope, msg);
      if (status != 0)
	return status;
      mu_envelope_set_sender (envelope, message_envelope_sender, msg);
      mu_envelope_set_date (envelope, message_envelope_date, msg);
      msg->envelope = envelope;
    }
  *penvelope = msg->envelope;
  return 0;
}

int
mu_message_set_envelope (mu_message_t msg, mu_envelope_t envelope, void *owner)
{
  if (msg == NULL)
    return EINVAL;
  if (msg->owner != owner)
    return EACCES;
  if (msg->envelope)
    mu_envelope_destroy (&msg->envelope, msg);
  msg->envelope = envelope;
  msg->flags |= MESSAGE_MODIFIED;
  return 0;
}

int
mu_message_get_attribute (mu_message_t msg, mu_attribute_t *pattribute)
{
  if (msg == NULL)
    return EINVAL;
  if (pattribute == NULL)
    return MU_ERR_OUT_PTR_NULL;
  if (msg->attribute == NULL)
    {
      mu_attribute_t attribute;
      int status = mu_attribute_create (&attribute, msg);
      if (status != 0)
	return status;
      msg->attribute = attribute;
    }
  *pattribute = msg->attribute;
  return 0;
}

int
mu_message_set_attribute (mu_message_t msg, mu_attribute_t attribute, void *owner)
{
  if (msg == NULL)
   return EINVAL;
  if (msg->owner != owner)
    return EACCES;
  if (msg->attribute)
    mu_attribute_destroy (&msg->attribute, owner);
  msg->attribute = attribute;
  msg->flags |= MESSAGE_MODIFIED;
  return 0;
}

int
mu_message_get_uid (mu_message_t msg, size_t *puid)
{
  if (msg == NULL)
    return EINVAL;
  if (msg->_get_uid)
    return msg->_get_uid (msg, puid);
  *puid = 0;
  return 0;
}

int
mu_message_get_uidl (mu_message_t msg, char *buffer, size_t buflen,
		     size_t *pwriten)
{
  mu_header_t header = NULL;
  size_t n = 0;
  int status;

  if (msg == NULL || buffer == NULL || buflen == 0)
    return EINVAL;

  buffer[0] = '\0';
  /* Try the function overload if error fallback.  */
  if (msg->_get_uidl)
    {
      status = msg->_get_uidl (msg, buffer, buflen, pwriten);
      if (status == 0)
	return status;
    }

  /* Be compatible with Qpopper ? qppoper saves the UIDL in "X-UIDL".
     We generate a chksum and save it in the header.  */
  mu_message_get_header (msg, &header);
  status = mu_header_get_value_unfold (header, "X-UIDL", buffer, buflen, &n);
  if (status != 0 || n == 0)
    {
      size_t uid = 0;
      struct mu_md5_ctx md5context;
      mu_stream_t stream = NULL;
      char buf[1024];
      unsigned char md5digest[16];
      char *tmp;
      n = 0;
      mu_message_get_uid (msg, &uid);
      mu_message_get_streamref (msg, &stream);
      mu_md5_init_ctx (&md5context);
      status = mu_stream_seek (stream, 0, MU_SEEK_SET, NULL);
      if (status == 0)
	{
	  while (mu_stream_read (stream, buf, sizeof (buf), &n) == 0
		 && n > 0)
	    mu_md5_process_bytes (buf, n, &md5context);
	  mu_md5_finish_ctx (&md5context, md5digest);
	  tmp = buf;
	  for (n = 0; n < 16; n++, tmp += 2)
	    sprintf (tmp, "%02x", md5digest[n]);
	  *tmp = '\0';
	  /* POP3 rfc says that an UID should not be longer than 70.  */
	  snprintf (buf + 32, 70, ".%lu.%lu", (unsigned long)time (NULL), 
		    (unsigned long) uid);

	  mu_header_set_value (header, "X-UIDL", buf, 1);
	  buflen--; /* leave space for the NULL.  */
	  strncpy (buffer, buf, buflen)[buflen] = '\0';
	}
      mu_stream_destroy (&stream);
    }
  return status;
}

int
mu_message_get_qid (mu_message_t msg, mu_message_qid_t *pqid)
{
  if (msg == NULL)
    return EINVAL;
  if (!msg->_get_qid)
    return ENOSYS;
  return msg->_get_qid (msg, pqid);
}
    
int
mu_message_set_qid (mu_message_t msg,
		    int (*_get_qid) (mu_message_t, mu_message_qid_t *),
		    void *owner)
{
  if (msg == NULL)
    return EINVAL;
  if (msg->owner != owner)
    return EACCES;
  msg->_get_qid = _get_qid;
  return 0;
}

int
mu_message_set_uid (mu_message_t msg, int (*_get_uid) (mu_message_t, size_t *),
		    void *owner)
{
  if (msg == NULL)
    return EINVAL;
  if (msg->owner != owner)
    return EACCES;
  msg->_get_uid = _get_uid;
  return 0;
}

int
mu_message_set_uidl (mu_message_t msg,
		  int (* _get_uidl) (mu_message_t, char *, size_t, size_t *),
		  void *owner)
{
  if (msg == NULL)
    return EINVAL;
  if (msg->owner != owner)
    return EACCES;
  msg->_get_uidl = _get_uidl;
  return 0;
}

int
mu_message_set_is_multipart (mu_message_t msg,
			  int (*_is_multipart) (mu_message_t, int *),
			  void *owner)
{
  if (msg == NULL)
    return EINVAL;
  if (msg->owner != owner)
    return EACCES;
  msg->_is_multipart = _is_multipart;
  return 0;
}

int
mu_message_is_multipart (mu_message_t msg, int *pmulti)
{
  if (msg && pmulti)
    {
      if (msg->_is_multipart)
	return msg->_is_multipart (msg, pmulti);
      if (msg->mime == NULL)
	{
	  int status = mu_mime_create (&msg->mime, msg, 0);
	  if (status != 0)
	    return 0;
	}
      *pmulti = mu_mime_is_multipart(msg->mime);
    }
  return 0;
}

int
mu_message_get_num_parts (mu_message_t msg, size_t *pparts)
{
  if (msg == NULL || pparts == NULL)
    return EINVAL;

  if (msg->_get_num_parts)
    return msg->_get_num_parts (msg, pparts);

  if (msg->mime == NULL)
    {
      int status = mu_mime_create (&msg->mime, msg, 0);
      if (status != 0)
	return status;
    }
  return mu_mime_get_num_parts (msg->mime, pparts);
}

int
mu_message_set_get_num_parts (mu_message_t msg,
			   int (*_get_num_parts) (mu_message_t, size_t *),
			   void *owner)
{
  if (msg == NULL)
    return EINVAL;
  if (msg->owner != owner)
    return EACCES;
  msg->_get_num_parts = _get_num_parts;
  return 0;
}

int
mu_message_get_part (mu_message_t msg, size_t part, mu_message_t *pmsg)
{
  if (msg == NULL || pmsg == NULL)
    return EINVAL;

  /* Overload.  */
  if (msg->_get_part)
    return msg->_get_part (msg, part, pmsg);

  if (msg->mime == NULL)
    {
      int status = mu_mime_create (&msg->mime, msg, 0);
      if (status != 0)
	return status;
    }
  return mu_mime_get_part (msg->mime, part, pmsg);
}

int
mu_message_set_get_part (mu_message_t msg, int (*_get_part)
		      (mu_message_t, size_t, mu_message_t *),
		      void *owner)
{
  if (msg == NULL)
    return EINVAL;
  if (msg->owner != owner)
    return EACCES;
  msg->_get_part = _get_part;
  return 0;
}

int
mu_message_get_observable (mu_message_t msg, mu_observable_t *pobservable)
{
  if (msg == NULL || pobservable == NULL)
    return EINVAL;

  if (msg->observable == NULL)
    {
      int status = mu_observable_create (&msg->observable, msg);
      if (status != 0)
	return status;
    }
  *pobservable = msg->observable;
  return 0;
}

int
mu_message_save_to_mailbox (mu_message_t msg, 
                            mu_debug_t debug,
			    const char *toname, int perms)
{
  int rc = 0;
  mu_mailbox_t to = 0;

  if ((rc = mu_mailbox_create_default (&to, toname)))
    {
      MU_DEBUG2 (debug, MU_DEBUG_ERROR,
		 "mu_mailbox_create_default (%s) failed: %s\n", toname,
		 mu_strerror (rc));
      goto end;
    }

  if (debug && (rc = mu_mailbox_set_debug (to, debug)))
	goto end;

  if ((rc = mu_mailbox_open (to,
			     MU_STREAM_WRITE | MU_STREAM_CREAT
			     | (perms & MU_STREAM_IMASK))))
    {
      MU_DEBUG2 (debug, MU_DEBUG_ERROR,
		 "mu_mailbox_open (%s) failed: %s\n", toname,
		 mu_strerror (rc));
      goto end;
    }

  if ((rc = mu_mailbox_append_message (to, msg)))
    {
      MU_DEBUG2 (debug, MU_DEBUG_ERROR,
		 "mu_mailbox_append_message (%s) failed: %s\n", toname,
		 mu_strerror (rc));
      goto end;
    }

end:

  if (!rc)
    {
      if ((rc = mu_mailbox_close (to)))
	MU_DEBUG2 (debug, MU_DEBUG_ERROR,
		   "mu_mailbox_close (%s) failed: %s\n", toname,
		   mu_strerror (rc));
    }
  else
    mu_mailbox_close (to);

  mu_mailbox_destroy (&to);

  return rc;
}

