/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002, 2004, 2005  Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

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
#include <ctype.h>
#include <pwd.h>

#include "md5.h"

#include <message0.h>

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

#define MESSAGE_MODIFIED 0x10000;

static int message_read   (stream_t is, char *buf, size_t buflen,
			   off_t off, size_t *pnread );
static int message_write  (stream_t os, const char *buf, size_t buflen,
			   off_t off, size_t *pnwrite);
static int message_get_transport2 (stream_t stream, mu_transport_t *pin, 
                                   mu_transport_t *pout);
static int message_sender (envelope_t envelope, char *buf, size_t len,
			   size_t *pnwrite);
static int message_date   (envelope_t envelope, char *buf, size_t len,
			   size_t *pnwrite);
static int message_stream_size (stream_t stream, off_t *psize);
static int message_header_fill (header_t header, char *buffer,
			        size_t buflen, off_t off,
				size_t * pnread);
static int message_body_read (stream_t stream,  char *buffer,
			      size_t n, off_t off, size_t *pn);

/*  Allocate ressources for the message_t.  */
int
message_create (message_t *pmsg, void *owner)
{
  message_t msg;
  int status;

  if (pmsg == NULL)
    return MU_ERR_OUT_PTR_NULL;
  msg = calloc (1, sizeof (*msg));
  if (msg == NULL)
    return ENOMEM;
  status = monitor_create (&(msg->monitor), 0, msg);
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
message_destroy (message_t *pmsg, void *owner)
{
  if (pmsg && *pmsg)
    {
      message_t msg = *pmsg;
      monitor_t monitor = msg->monitor;
      int destroy_lock = 0;

      monitor_wrlock (monitor);
      msg->ref--;
      if ((msg->owner && msg->owner == owner)
	  || (msg->owner == NULL && msg->ref <= 0))
	{
	  destroy_lock =  1;
	  /* Notify the listeners.  */
	  /* FIXME: to be removed since we do not support this event.  */
	  if (msg->observable)
	    {
	      observable_notify (msg->observable, MU_EVT_MESSAGE_DESTROY);
	      observable_destroy (&(msg->observable), msg);
	    }

	  /* Envelope.  */
	  if (msg->envelope)
	    envelope_destroy (&(msg->envelope), msg);

	  /* Header.  */
	  if (msg->header)
	    header_destroy (&(msg->header), msg);

	  /* Body.  */
	  if (msg->body)
	    body_destroy (&(msg->body), msg);

	  /* Attribute.  */
	  if (msg->attribute)
	    attribute_destroy (&(msg->attribute), msg);

	  /* Stream.  */
	  if (msg->stream)
	    stream_destroy (&(msg->stream), msg);

	  /*  Mime.  */
	  if (msg->mime)
	    mime_destroy (&(msg->mime));

	  /* Loose the owner.  */
	  msg->owner = NULL;

	  /* Mailbox maybe created floating i.e they were created
	     implicitely by the message when doing something like:
	     message_create (&msg, "pop://localhost/msgno=2", NULL);
	     message_create (&msg, "imap://localhost/alain;uid=xxxxx", NULL);
	     althought the semantics about this is still flaky we our
	     making some provisions here for it.
	     if (msg->floating_mailbox && msg->mailbox)
	     mailbox_destroy (&(msg->mailbox));
	  */

	  if (msg->ref == 0)
	    free (msg);
	}
      monitor_unlock (monitor);
      if (destroy_lock)
	monitor_destroy (&monitor, msg);
      /* Loose the link */
      *pmsg = NULL;
    }
}

int
message_create_copy (message_t *to, message_t from)
{
  int status = 0;
  stream_t fromstr = NULL;
  stream_t tostr = NULL;
  off_t off = 0;
  size_t n = 0;
  char buf[512];

  if (!to)
    return MU_ERR_OUT_PTR_NULL;
  if (!from)
    return EINVAL;

  if((status = message_create (to, NULL)))
    return status;

  message_get_stream (from, &fromstr);
  message_get_stream (*to, &tostr);

  while (
      (status = stream_readline (fromstr, buf, sizeof(buf), off, &n)) == 0
	 &&
      n > 0
      )
    {
      stream_write (tostr, buf, n, off, NULL);
      off += n;
    }

  if(status)
    message_destroy(to, NULL);
  
  return status;
}

int
message_ref (message_t msg)
{
  if (msg)
    {
      monitor_wrlock (msg->monitor);
      msg->ref++;
      monitor_unlock (msg->monitor);
    }
  return 0;
}

void *
message_get_owner (message_t msg)
{
  return (msg == NULL) ? NULL : msg->owner;
}

int
message_is_modified (message_t msg)
{
  int mod = 0;
  if (msg)
    {
      mod |= header_is_modified (msg->header);
      mod |= attribute_is_modified (msg->attribute);
      mod |= body_is_modified (msg->body);
      mod |= msg->flags;
    }
  return mod;
}

int
message_clear_modified (message_t msg)
{
  if (msg)
    {
      if (msg->header)
	header_clear_modified (msg->header);
      if (msg->attribute)
	attribute_clear_modified (msg->attribute);
      if (msg->body)
	body_clear_modified (msg->body);
      msg->flags &= ~MESSAGE_MODIFIED;
    }
  return 0;
}

int
message_get_mailbox (message_t msg, mailbox_t *pmailbox)
{
  if (msg == NULL)
    return EINVAL;
  if (pmailbox == NULL)
    return MU_ERR_OUT_PTR_NULL;
  *pmailbox = msg->mailbox;
  return 0;
}

int
message_set_mailbox (message_t msg, mailbox_t mailbox, void *owner)
{
  if (msg == NULL)
    return EINVAL;
  if (msg->owner != owner)
    return EACCES;
  msg->mailbox = mailbox;
  return 0;
}

int
message_get_header (message_t msg, header_t *phdr)
{
  if (msg == NULL)
    return EINVAL;
  if (phdr == NULL)
    return MU_ERR_OUT_PTR_NULL;

  /* Is it a floating mesg */
  if (msg->header == NULL)
    {
      header_t header;
      int status = header_create (&header, NULL, 0, msg);
      if (status != 0)
	return status;
      if (msg->stream)
	{
	  /* Was it created by us?  */
	  message_t mesg = stream_get_owner (msg->stream);
	  if (mesg != msg)
	    header_set_fill (header, message_header_fill, msg);
	}
      msg->header = header;
    }
  *phdr = msg->header;
  return 0;
}

int
message_set_header (message_t msg, header_t hdr, void *owner)
{
  if (msg == NULL )
    return EINVAL;
  if (msg->owner != owner)
     return EACCES;
  /* Make sure we destroy the old if it was own by the mesg */
  /* FIXME:  I do not know if somebody has already a ref on this ? */
  if (msg->header)
    header_destroy (&(msg->header), msg);
  msg->header = hdr;
  msg->flags |= MESSAGE_MODIFIED;
  return 0;
}

int
message_get_body (message_t msg, body_t *pbody)
{
  if (msg == NULL)
    return EINVAL;
  if (pbody == NULL)
    return MU_ERR_OUT_PTR_NULL;

  /* Is it a floating mesg.  */
  if (msg->body == NULL)
    {
      body_t body;
      int status = body_create (&body, msg);
      if (status != 0)
	return status;
      /* If a stream is already set use it to create the body stream.  */
      if (msg->stream)
	{
	  /* Was it created by us?  */
	  message_t mesg = stream_get_owner (msg->stream);
	  if (mesg != msg)
	    {
	      stream_t stream;
	      int flags = 0;
	      stream_get_flags (msg->stream, &flags);
	      if ((status = stream_create (&stream, flags, body)) != 0)
		{
		  body_destroy (&body, msg);
		  return status;
		}
	      stream_set_read (stream, message_body_read, body);
	      stream_setbufsiz (stream, 128);
	      body_set_stream (body, stream, msg);
	    }
	}
      msg->body = body;
    }
  *pbody = msg->body;
  return 0;
}

int
message_set_body (message_t msg, body_t body, void *owner)
{
  if (msg == NULL )
    return EINVAL;
  if (msg->owner != owner)
    return EACCES;
  /* Make sure we destoy the old if it was own by the mesg.  */
  /* FIXME:  I do not know if somebody has already a ref on this ? */
  if (msg->body)
    body_destroy (&(msg->body), msg);
  msg->body = body;
  msg->flags |= MESSAGE_MODIFIED;
  return 0;
}

int
message_set_stream (message_t msg, stream_t stream, void *owner)
{
  if (msg == NULL)
    return EINVAL;
  if (msg->owner != owner)
    return EACCES;
  /* Make sure we destoy the old if it was own by the mesg.  */
  /* FIXME:  I do not know if somebody has already a ref on this ? */
  if (msg->stream)
    stream_destroy (&(msg->stream), msg);
  msg->stream = stream;
  msg->flags |= MESSAGE_MODIFIED;
  return 0;
}

int
message_get_stream (message_t msg, stream_t *pstream)
{
  if (msg == NULL)
    return EINVAL;
  if (pstream == NULL)
    return MU_ERR_OUT_PTR_NULL;

  if (msg->stream == NULL)
    {
      stream_t stream;
      int status;
      status = stream_create (&stream, MU_STREAM_RDWR, msg);
      if (status != 0)
	return status;
      stream_set_read (stream, message_read, msg);
      stream_set_write (stream, message_write, msg);
      stream_set_get_transport2 (stream, message_get_transport2, msg);
      stream_set_size (stream, message_stream_size, msg);
      stream_set_flags (stream, MU_STREAM_RDWR);
      msg->stream = stream;
    }

  *pstream = msg->stream;
  return 0;
}

int
message_set_lines (message_t msg, int (*_lines)
		   (message_t, size_t *), void *owner)
{
  if (msg == NULL)
    return EINVAL;
  if (msg->owner != owner)
    return EACCES;
  msg->_lines = _lines;
  return 0;
}

int
message_lines (message_t msg, size_t *plines)
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
      if ( ( ret = header_lines (msg->header, &hlines) ) == 0 )
	      ret = body_lines (msg->body, &blines);
      *plines = hlines + blines;
    }
  return ret;
}

int
message_set_size (message_t msg, int (*_size)
		  (message_t, size_t *), void *owner)
{
  if (msg == NULL)
    return EINVAL;
  if (msg->owner != owner)
    return EACCES;
  msg->_size = _size;
  return 0;
}

int
message_size (message_t msg, size_t *psize)
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
      header_t hdr = NULL;
      body_t body = NULL;
      
      hsize = bsize = 0;
      message_get_header (msg, &hdr);
      message_get_body (msg, &body);
      if ( ( ret = header_size (hdr, &hsize) ) == 0 )
	ret = body_size (body, &bsize);
      *psize = hsize + bsize;
    }
  return ret;
}

int
message_get_envelope (message_t msg, envelope_t *penvelope)
{
  if (msg == NULL)
    return EINVAL;
  if (penvelope == NULL)
    return MU_ERR_OUT_PTR_NULL;

  if (msg->envelope == NULL)
    {
      envelope_t envelope;
      int status = envelope_create (&envelope, msg);
      if (status != 0)
	return status;
      envelope_set_sender (envelope, message_sender, msg);
      envelope_set_date (envelope, message_date, msg);
      msg->envelope = envelope;
    }
  *penvelope = msg->envelope;
  return 0;
}

int
message_set_envelope (message_t msg, envelope_t envelope, void *owner)
{
  if (msg == NULL)
    return EINVAL;
  if (msg->owner != owner)
    return EACCES;
  if (msg->envelope)
    envelope_destroy (&(msg->envelope), msg);
  msg->envelope = envelope;
  msg->flags |= MESSAGE_MODIFIED;
  return 0;
}

int
message_get_attribute (message_t msg, attribute_t *pattribute)
{
  if (msg == NULL)
    return EINVAL;
  if (pattribute == NULL)
    return MU_ERR_OUT_PTR_NULL;
  if (msg->attribute == NULL)
    {
      attribute_t attribute;
      int status = attribute_create (&attribute, msg);
      if (status != 0)
	return status;
      msg->attribute = attribute;
    }
  *pattribute = msg->attribute;
  return 0;
}

int
message_set_attribute (message_t msg, attribute_t attribute, void *owner)
{
  if (msg == NULL)
   return EINVAL;
  if (msg->owner != owner)
    return EACCES;
  if (msg->attribute)
    attribute_destroy (&(msg->attribute), owner);
  msg->attribute = attribute;
  msg->flags |= MESSAGE_MODIFIED;
  return 0;
}

int
message_get_uid (message_t msg, size_t *puid)
{
  if (msg == NULL)
    return EINVAL;
  if (msg->_get_uid)
    return msg->_get_uid (msg, puid);
  *puid = 0;
  return 0;
}

int
message_get_uidl (message_t msg, char *buffer, size_t buflen, size_t *pwriten)
{
  header_t header = NULL;
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
  message_get_header (msg, &header);
  status = header_get_value (header, "X-UIDL", buffer, buflen, &n);
  if (status == 0 && n > 0)
    {
      /* FIXME: Is this necessary ? I did not see so far a x-uidl message
	 that broken i.e.
	 X-UIDL:  <abaceekeke\n
	       jakdkjaja>
      */
      /* We need to collapse the header if it was mutiline.  e points to the
	 last char meaning in a C string that's '\0', s to the start. We also
	 remove the spesky '<' '>' if they are around.  */
      char *s, *e;
      for (s = buffer, e = buffer + n; s <= e; s++)
	{
	  if (isspace ((unsigned char)*s) || *s == '<' || *s == '>')
	    {
	      memmove (s, s + 1, e - (s + 1));
	      e -= 1;
	      *e = '\0';
	    }
	}
    }
  else
    {
      size_t uid = 0;
      struct md5_ctx md5context;
      stream_t stream = NULL;
      char buf[1024];
      off_t offset = 0;
      unsigned char md5digest[16];
      char *tmp;
      n = 0;
      message_get_uid (msg, &uid);
      message_get_stream (msg, &stream);
      md5_init_ctx (&md5context);
      while (stream_read (stream, buf, sizeof (buf), offset, &n) == 0
	     && n > 0)
	{
	  md5_process_bytes (buf, n, &md5context);
	  offset += n;
	}
      md5_finish_ctx (&md5context, md5digest);
      tmp = buf;
      for (n = 0; n < 16; n++, tmp += 2)
	sprintf (tmp, "%02x", md5digest[n]);
      *tmp = '\0';
      /* POP3 rfc says that an UID should not be longer than 70.  */
      snprintf (buf + 32, 70, ".%lu.%u", (unsigned long)time (NULL), uid);

      header_set_value (header, "X-UIDL", buf, 1);
      buflen--; /* leave space for the NULL.  */
      strncpy (buffer, buf, buflen)[buflen] = '\0';
    }
  return status;
}

int
message_set_uid (message_t msg, int (*_get_uid) (message_t, size_t *),
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
message_set_uidl (message_t msg,
		  int (* _get_uidl) (message_t, char *, size_t, size_t *),
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
message_set_is_multipart (message_t msg,
			  int (*_is_multipart) (message_t, int *),
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
message_is_multipart (message_t msg, int *pmulti)
{
  if (msg && pmulti)
    {
      if (msg->_is_multipart)
	return msg->_is_multipart (msg, pmulti);
      if (msg->mime == NULL)
	{
	  int status = mime_create (&(msg->mime), msg, 0);
	  if (status != 0)
	    return 0;
	}
      *pmulti = mime_is_multipart(msg->mime);
    }
  return 0;
}

int
message_get_num_parts (message_t msg, size_t *pparts)
{
  if (msg == NULL || pparts == NULL)
    return EINVAL;

  if (msg->_get_num_parts)
    return msg->_get_num_parts (msg, pparts);

  if (msg->mime == NULL)
    {
      int status = mime_create (&(msg->mime), msg, 0);
      if (status != 0)
	return status;
    }
  return mime_get_num_parts (msg->mime, pparts);
}

int
message_set_get_num_parts (message_t msg,
			   int (*_get_num_parts) (message_t, size_t *),
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
message_get_part (message_t msg, size_t part, message_t *pmsg)
{
  if (msg == NULL || pmsg == NULL)
    return EINVAL;

  /* Overload.  */
  if (msg->_get_part)
    return msg->_get_part (msg, part, pmsg);

  if (msg->mime == NULL)
    {
      int status = mime_create (&(msg->mime), msg, 0);
      if (status != 0)
	return status;
    }
  return mime_get_part (msg->mime, part, pmsg);
}

int
message_set_get_part (message_t msg, int (*_get_part)
		      (message_t, size_t, message_t *),
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
message_get_observable (message_t msg, observable_t *pobservable)
{
  if (msg == NULL || pobservable == NULL)
    return EINVAL;

  if (msg->observable == NULL)
    {
      int status = observable_create (&(msg->observable), msg);
      if (status != 0)
	return status;
    }
  *pobservable = msg->observable;
  return 0;
}

/* Implements the stream_read () on the message stream.  */
static int
message_read (stream_t is, char *buf, size_t buflen,
	      off_t off, size_t *pnread )
{
  message_t msg =  stream_get_owner (is);
  stream_t his, bis;
  size_t hread, hsize, bread, bsize;

  if (msg == NULL)
    return EINVAL;

  bsize = hsize = bread = hread = 0;
  his = bis = NULL;

  header_size (msg->header, &hsize);
  body_size (msg->body, &bsize);

  /* On some remote sever (POP) the size of the header and body is not known
     until you start reading them.  So by checking hsize == bsize == 0,
     this kludge is a way of detecting the anomalie and start by the
     header.  */
  if ((size_t)off < hsize || (hsize == 0 && bsize == 0))
    {
      header_get_stream (msg->header, &his);
      stream_read (his, buf, buflen, off, &hread);
    }
  else
    {
      body_get_stream (msg->body, &bis);
      stream_read (bis, buf, buflen, off - hsize, &bread);
    }

  if (pnread)
    *pnread = hread + bread;
  return 0;
}

/* Implements the stream_write () on the message stream.  */
static int
message_write (stream_t os, const char *buf, size_t buflen,
	       off_t off, size_t *pnwrite)
{
  message_t msg = stream_get_owner (os);
  int status = 0;
  size_t bufsize = buflen;

  if (msg == NULL)
    return EINVAL;

  /* Skip the obvious.  */
  if (buf == NULL || buflen == 0)
    {
      if (pnwrite)
	*pnwrite = 0;
      return 0;
    }

  if (!msg->hdr_done)
    {
      size_t len;
      char *nl;
      header_t header = NULL;
      stream_t hstream = NULL;
      message_get_header (msg, &header);
      header_get_stream (header, &hstream);
      while (!msg->hdr_done && (nl = memchr (buf, '\n', buflen)) != NULL)
	{
	  len = nl - buf + 1;
	  status = stream_write (hstream, buf, len, msg->hdr_buflen, NULL);
	  if (status != 0)
	    return status;
	  msg->hdr_buflen += len;
	  /* We detect an empty line .i.e "^\n$" this signal the end of the
	     header.  */
	  if (buf == nl)
	    msg->hdr_done = 1;
	  buf = nl + 1;
	  buflen -= len;
	}
    }

  /* Message header is not complete but was not a full line.  */
  if (!msg->hdr_done && buflen > 0)
    {
      header_t header = NULL;
      stream_t hstream = NULL;
      message_get_header (msg, &header);
      header_get_stream (header, &hstream);
      status = stream_write (hstream, buf, buflen, msg->hdr_buflen, NULL);
      if (status != 0)
	return status;
      msg->hdr_buflen += buflen;
      buflen = 0;
    }
  else if (buflen > 0) /* In the body.  */
    {
      stream_t bs;
      body_t body;
      size_t written = 0;
      if ((status = message_get_body (msg, &body)) != 0 ||
	  (status = body_get_stream (msg->body, &bs)) != 0)
	{
	  msg->hdr_buflen = msg->hdr_done = 0;
	  return status;
	}
      if (off < (off_t)msg->hdr_buflen)
	off = 0;
      else
	off -= msg->hdr_buflen;
      status = stream_write (bs, buf, buflen, off, &written);
      buflen -= written;
    }
  if (pnwrite)
    *pnwrite = bufsize - buflen;
  return status;
}

static int
message_get_transport2 (stream_t stream, mu_transport_t *pin, mu_transport_t *pout)
{
  message_t msg = stream_get_owner (stream);
  body_t body;
  stream_t is;

  if (msg == NULL)
    return EINVAL;
  if (pout)
    *pout = NULL;

  /* Probably being lazy, then create a body for the stream.  */
  if (msg->body == NULL)
    {
      int status = body_create (&body, msg);
      if (status != 0 )
	return status;
      msg->body = body;
    }
  else
      body = msg->body;

  body_get_stream (body, &is);
  return stream_get_transport2 (is, pin, pout);
}

/* Implements the stream_stream_size () on the message stream.  */
static int
message_stream_size (stream_t stream, off_t *psize)
{
  message_t msg = stream_get_owner (stream);
  return message_size (msg, (size_t*) psize);
}

static int
message_date (envelope_t envelope, char *buf, size_t len, size_t *pnwrite)
{
  message_t msg = envelope_get_owner (envelope);
  time_t t;
  size_t n;

  if (msg == NULL)
    return EINVAL;

  /* FIXME: extract the time from "Date:".  */

  /* Catch all.  */
  /* FIXME: ctime() is not thread safe use strftime().  */
  t = time (NULL);
  n = strlen (ctime (&t));

  if (buf == NULL || len == 0)
    {
      if (pnwrite)
	*pnwrite = n;
      return 0;
    }
  n = (n > len) ? len : n;
  strncpy (buf, ctime (&t), n);
  buf [n] = '\0';
  if (pnwrite)
    *pnwrite = n;
  return 0;
}

static int
message_sender (envelope_t envelope, char *buf, size_t len, size_t *pnwrite)
{
  message_t msg = envelope_get_owner (envelope);
  header_t header = NULL;
  size_t n = 0;
  int status;

  if (msg == NULL)
    return EINVAL;

  /* Can it be extracted from the From:  */
  message_get_header (msg, &header);
  status = header_get_value (header, MU_HEADER_FROM, NULL, 0, &n);
  if (status == 0 && n != 0)
    {
      char *sender;
      address_t address = NULL;
      sender = calloc (1, n + 1);
      if (sender == NULL)
	return ENOMEM;
      header_get_value (header, MU_HEADER_FROM, sender, n + 1, NULL);
      if (address_create (&address, sender) == 0)
	address_get_email (address, 1, buf, n + 1, pnwrite);
      free (sender);
      address_destroy (&address);
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

static int
message_header_fill (header_t header, char *buffer, size_t buflen,
		     off_t off, size_t * pnread)
{
  int status = 0;
  message_t msg = header_get_owner (header);
  stream_t stream = NULL;
  size_t nread = 0;

  /* Noop.  */
  if (buffer == NULL || buflen == 0)
    {
      if (pnread)
        *pnread = nread;
      return 0;
    }

  if (!msg->hdr_done)
    {
      status = message_get_stream (msg, &stream);
      if (status == 0)
	{
	  /* Position the file pointer and the buffer.  */
	  status = stream_readline (stream, buffer, buflen, off, &nread);
	  /* Detect the end of the headers. */
	  if (nread  && buffer[0] == '\n' && buffer[1] == '\0')
	    {
	      msg->hdr_done = 1;
	    }
	  msg->hdr_buflen += nread;
	}
    }

  if (pnread)
    *pnread = nread;

  return status;
}

static int
message_body_read (stream_t stream,  char *buffer, size_t n, off_t off,
		   size_t *pn)
{
  body_t body = stream_get_owner (stream);
  message_t msg = body_get_owner (body);
  size_t nread = 0;
  header_t header = NULL;
  stream_t bstream = NULL;
  size_t size = 0;
  int status;

  message_get_header (msg, &header);
  status = header_size (msg->header, &size);
  if (status == 0)
    {
      message_get_stream (msg, &bstream);
      status = stream_read (bstream, buffer, n, size + off, &nread);
    }
  if (pn)
    *pn = nread;
  return status;
}

int
message_save_to_mailbox (message_t msg, ticket_t ticket, mu_debug_t debug,
			 const char *toname)
{
  int rc = 0;
  mailbox_t to = 0;

  if ((rc = mailbox_create_default (&to, toname)))
    {
      mu_debug_print (debug, MU_DEBUG_TRACE,
		      _("mailbox_create_default (%s) failed: %s\n"), toname,
		      mu_strerror (rc));
      goto end;
    }

  if (debug && (rc = mailbox_set_debug (to, debug)))
	goto end;

  if (ticket)
    {
      folder_t folder = NULL;

      if ((rc = mailbox_get_folder (to, &folder)))
	goto end;

      /* FIXME: not all mailboxes have folders, thus this hack. */
      if (folder)
	{
	  authority_t auth = NULL;
	  if ((rc = folder_get_authority (folder, &auth)))
	    goto end;

	  /* FIXME: not all folders have authentication, thus this hack. */
	  if (auth && (rc = authority_set_ticket (auth, ticket)))
	    goto end;
	}
    }

  if ((rc = mailbox_open (to, MU_STREAM_WRITE | MU_STREAM_CREAT)))
    {
      mu_debug_print (debug, MU_DEBUG_TRACE,
		      _("mailbox_open (%s) failed: %s\n"), toname,
		      mu_strerror (rc));
      goto end;
    }

  if ((rc = mailbox_append_message (to, msg)))
    {
      mu_debug_print (debug, MU_DEBUG_TRACE,
		      _("mailbox_append_message (%s) failed: %s\n"), toname,
		      mu_strerror (rc));
      goto end;
    }

end:

  if (!rc)
    {
      if ((rc = mailbox_close (to)))
	{
	  mu_debug_print (debug, MU_DEBUG_TRACE,
			  _("mailbox_close (%s) failed: %s\n"), toname,
			  mu_strerror (rc));
	}
    }
  else
    {
      mailbox_close (to);
    }

  mailbox_destroy (&to);

  return rc;
}

