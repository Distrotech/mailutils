/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000 Free Software Foundation, Inc.

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


#include <stream0.h>
#include <message0.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <string.h>

static int extract_addr(const char *s, size_t n, char **presult,
		       size_t *pnwrite);
static int message_read (stream_t is, char *buf, size_t buflen,
			 off_t off, size_t *pnread );
static int message_write (stream_t os, const char *buf, size_t buflen,
			  off_t off, size_t *pnwrite);
static int message_get_fd (stream_t stream, int *pfd);

int
message_create (message_t *pmsg, void *owner)
{
  message_t msg;

  if (pmsg == NULL)
    return EINVAL;
  msg = calloc (1, sizeof (*msg));
  if (msg == NULL)
    return ENOMEM;
  msg->owner = owner;
  *pmsg = msg;
  return 0;
}

void
message_destroy (message_t *pmsg, void *owner)
{
  if (pmsg && *pmsg)
    {
      message_t msg = *pmsg;

      if (msg->owner == owner)
	{
	  /* notify the listeners */
	  if (msg->event_num)
	    {
	      message_notification (msg, MU_EVT_MSG_DESTROY);
	      free (msg->event);
	    }
	  /* header */
	  header_destroy (&(msg->header), owner);
	  /* attribute */
	  attribute_destroy (&(msg->attribute), owner);
	  /* stream */
	  stream_destroy (&(msg->stream), owner);

	  /* if sometype of floating/temporary message */
	  body_destroy (&(msg->body), owner);
	  /* notifications are done */

	  /* check again for resurrection before free()ing
	   * the memory maybe it was clone, if yes we can not
	   * free the pointer.
	   *
	   */
	  free (msg);
	}
      /* loose the link */
      *pmsg = NULL;
    }
}

int
message_get_header (message_t msg, header_t *phdr)
{
  if (msg == NULL || phdr == NULL)
    return EINVAL;

  /* is it a floating mesg */
  if (msg->header == NULL && msg->owner == NULL)
    {
      header_t header;
      int status = header_create (&header, NULL, 0, msg);
      if (status != 0)
	return status;
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
  /* make sure we destoy the old if it was own by the mesg */
  header_destroy (&(msg->header), msg);
  msg->header = hdr;
  return 0;
}

int
message_get_body (message_t msg, body_t *pbody)
{
  if (msg == NULL || pbody == NULL)
    return EINVAL;

  /* is it a floating mesg */
  if (msg->body == NULL)
    {
      body_t body;
      int status = body_create (&body, msg);
      if (status != 0)
	return status;
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
  /* make sure we destoy the old if it was own by the mesg */
  body_destroy (&(msg->body), msg);
  msg->body = body;
  return 0;
}

int
message_set_stream (message_t msg, stream_t stream, void *owner)
{
  if (msg == NULL)
    return EINVAL;
  if (msg->owner != owner)
    return EACCES;
  msg->stream = stream;
  return 0;
}

int
message_get_stream (message_t msg, stream_t *pstream)
{
  if (msg == NULL || pstream == NULL)
    return EINVAL;

  if (msg->stream == NULL)
    {
      stream_t stream;
      int status;
      status = stream_create (&stream, MU_STREAM_RDWR, msg);
      if (status != 0)
	return status;
      stream_set_read (stream, message_read, msg);
      stream_set_write (stream, message_write, msg);
      stream_set_fd (stream, message_get_fd, msg);
      stream_set_flags (stream, MU_STREAM_RDWR, msg);
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
  if (msg == NULL)
    return EINVAL;
  /* Overload */
  if (msg->_lines)
    return msg->_lines (msg, plines);
  if (plines)
    {
      hlines = blines = 0;
      header_lines (msg->header, &hlines);
      body_lines (msg->body, &blines);
      *plines = hlines + blines;
    }
  return 0;
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
  if (msg == NULL)
    return EINVAL;
  /* Overload ? */
  if (msg->_size)
    return msg->_size (msg, psize);
  if (psize)
    {
      hsize = bsize = 0;
      header_size (msg->header, &hsize);
      body_size (msg->body, &bsize);
      *psize = hsize + bsize;
    }
  return 0;
}

int
message_set_from (message_t msg,
		  int (*_from) __P ((message_t, char *, size_t, size_t*)),
		  void *owner)
{
  if (msg == NULL)
    return EINVAL;
  if (msg->owner != owner)
    return EACCES;
  msg->_from = _from;
  return 0;
}

int
message_from (message_t msg, char *buf, size_t len, size_t *pnwrite)
{
  header_t header = NULL;
  size_t n = 0;
  int status;

  if (msg == NULL)
    return EINVAL;

  /* did they provide a way to get it */
  if (msg->_from)
    return msg->_from (msg, buf, len, pnwrite);

  /* can it be extracted from the From: */
  message_get_header (msg, &header);
  status = header_get_value (header, MU_HEADER_FROM, NULL, 0, &n);
  if (status == 0 && n != 0)
    {
      char *from = calloc (1, n + 1);
      char *addr;
      header_get_value (header, MU_HEADER_FROM, from, n + 1, NULL);
      if (extract_addr (from, n, &addr, &n) == 0)
	{
	  n = (n > len) ? len : n;
	  if (buf && len > 0)
	    {
	      memcpy (buf, addr, n);
	      buf[n] = '\0';
	    }
	  free (addr);
	  free (from);
	  if (pnwrite)
	    *pnwrite = n;
	  return 0;
	}
    }
  else if (status == EAGAIN)
    return status;

  /* oops */
  n = (7 > len) ? len: 7;
  if (buf && len > 0)
    {
      memcpy (buf, "unknown", n);
      buf [n] = '\0';
    }

  if (pnwrite)
    *pnwrite = n;
  return 0;
}

int
message_set_received (message_t msg, int (*_received)
		      __P ((message_t, char *, size_t , size_t *)),
		      void *owner)
{
  if (msg == NULL)
    return EINVAL;
  if (msg->owner != owner)
    return EACCES;
  msg->_received = _received;
  return 0;
}

int
message_received (message_t msg, char *buf, size_t len, size_t *pnwrite)
{
  time_t t;
  size_t n;
  if (msg == NULL)
    return EINVAL;
  /* is it provided */
  if (msg->_received)
    return msg->_received (msg, buf, len, pnwrite);

  /* FIXME: extract the time from "Date:" */

  /* catch all */
  /* FIXME: ctime() is not thread safe use strftime() */
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

int
message_get_attribute (message_t msg, attribute_t *pattribute)
{
  if (msg == NULL || pattribute == NULL)
   return EINVAL;
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
  attribute_destroy (&(msg->attribute), owner);
  msg->attribute = attribute;
  return 0;
}

int
message_get_uidl (message_t msg, char *buffer, size_t buflen, size_t *pwritten)
{
  header_t header = NULL;
  if (msg == NULL || buffer == NULL || buflen == 0)
    return EINVAL;

  buffer[0] = '0';
  if (msg->_get_uidl)
    return msg->_get_uidl (msg, buffer, buflen, pwritten);

  message_get_header (msg, &header);
  return header_get_value (header, "X-UIDL", buffer, buflen, pwritten);
}

int
message_set_uidl (message_t msg, int (* _get_uidl)
		  __P ((message_t msg, char *buffer, size_t buflen, size_t *pwritten)), void *owner)
{
  if (msg == NULL)
    return EINVAL;
  if (msg->owner != owner)
    return EACCES;
  msg->_get_uidl = _get_uidl;
  return 0;
}

int
message_set_is_multipart (message_t msg, int (*_is_multipart)
			  __P ((message_t, int *)), void *owner)
{
  if (msg == NULL)
    return EINVAL;
  if (msg->owner != owner)
    return EACCES;
  msg->_is_multipart = _is_multipart;
  return 0;
}

int
message_is_multipart (message_t msg, int *p_is_mp)
{
  header_t header;
  int status;
  size_t len = 0;

  if (msg == NULL || p_is_mp)
    return EINVAL;

  message_get_header(msg, &header);
  status = header_get_value (header, "Content-Type", NULL, 0, &len);
  if (status == 0)
    {
      char *content_type = calloc (len + 1, sizeof (char));
      if (content_type == NULL)
	return ENOMEM;
      status = header_get_value (header, "Content-Type", content_type,
				 len, NULL);
      *p_is_mp = strncasecmp ("multipart", content_type,
			      strlen ("multipart")) ? 0: 1;
      free (content_type);
    }
  else
    *p_is_mp = 0;
  return status;
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
message_set_get_num_parts (message_t msg, int (*_get_num_parts)
			   __P ((message_t, size_t *)), void *owner)
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
		      __P ((message_t, size_t, message_t *)),
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
message_register (message_t msg, size_t type,
		  int (*action) (size_t typ, void *arg), void *arg)
{
  event_t event;
  size_t i;
  if (msg == NULL || action == NULL || type == 0)
    return EINVAL;

  /* find a free spot */
  for (i = 0; i < msg->event_num; i++)
    {
      event = &(msg->event[i]);
      if (event->_action == NULL)
	{
	  event->type = type;
	  event->_action = action;
	  event->arg = arg;
	  return 0;
	}
    }
  event = realloc (msg->event, (msg->event_num + 1)*sizeof (*event));
  if (event == NULL)
      return ENOMEM;
  msg->event = event;
  event[msg->event_num]._action = action;
  event[msg->event_num].type = type;
  event[msg->event_num].arg = arg;
  msg->event_num++;
  return 0;
}

int
message_deregister (message_t msg, void *action)
{
  size_t i;
  event_t event;
  if (msg == NULL || action == NULL)
    return EINVAL;

  for (i = 0; i < msg->event_num; i++)
    {
      event = &(msg->event[i]);
      if ((int)event->_action == (int)action)
	{
	  event->type = 0;
	  event->_action = NULL;
	  event->arg = NULL;
	  return 0;
	}
    }
  return ENOENT;
}

void
message_notification (message_t msg, size_t type)
{
  size_t i;
  event_t event;
  for (i = 0; i < msg->event_num; i++)
    {
      event = &(msg->event[i]);
      if ((event->_action) &&  (event->type & type))
        event->_action (type, event->arg);
    }
}

static int
message_read (stream_t is, char *buf, size_t buflen,
	      off_t off, size_t *pnread )
{
  message_t msg =  is->owner;
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

static int
message_write (stream_t os, const char *buf, size_t buflen,
	       off_t off, size_t *pnwrite)
{
  message_t msg;
  int status = 0;
  size_t bufsize = buflen;

  if (os == NULL || (msg = os->owner) == NULL)
    return EINVAL;

  /* skip the obvious */
  if (buf == NULL || *buf == '\0' || buflen == 0)
    {
      if (pnwrite)
	*pnwrite = 0;
      return 0;
    }

  if (!msg->hdr_done)
    {
      size_t len;
      char *nl;
      char *thdr;
      while (!msg->hdr_done && (nl = memchr (buf, '\n', buflen)) != NULL)
	{
	  len = nl - buf + 1;
	  /* allocate more buffer to hold the header */
	  thdr = realloc (msg->hdr_buf, msg->hdr_buflen + len);
	  if (thdr == NULL)
	    {
	      free (msg->hdr_buf);
	      msg->hdr_buf = NULL;
	      msg->hdr_buflen = 0;
	      return ENOMEM;
	    }
	  else
	    msg->hdr_buf = thdr;
	  memcpy (msg->hdr_buf + msg->hdr_buflen, buf, len);
	  msg->hdr_buflen += len;
	  /* we detect an empty line .i.e "^\n$" this signal the end
	   * of the header */
	  if (buf == nl)
	    {
	      header_destroy (&(msg->header), msg);
	      status = header_create (&(msg->header), msg->hdr_buf,
				      msg->hdr_buflen, msg);
	      free (msg->hdr_buf);
	      msg->hdr_buf = NULL;
	      if (status != 0)
		{
		  msg->hdr_buflen = 0;
		  return status;
		}
	      msg->hdr_done = 1;
	    }
	  buf = nl + 1;
	  buflen -= len;
	}
    }

  /* message header is not complete but was not a full line */
  if (!msg->hdr_done && buflen > 0)
    {
      char *thdr = realloc (msg->hdr_buf, msg->hdr_buflen + buflen);
      if (thdr == NULL)
	{
	  free (msg->hdr_buf);
	  msg->hdr_buf = NULL;
	  msg->hdr_buflen = 0;
	  return ENOMEM;
	}
      else
	msg->hdr_buf = thdr;
      memcpy (msg->hdr_buf + msg->hdr_buflen, buf, buflen);
      msg->hdr_buflen += buflen;
      buflen = 0;
    }
  else if (buflen > 0) /* in the body */
    {
      stream_t bs;
      body_t body;
      size_t written = 0;
      if ((status = message_get_body (msg, &body)) != 0 ||
	  (status = body_get_stream (msg->body, &bs)) != 0)
	{
	  free (msg->hdr_buf);
	  msg->hdr_buf = NULL;
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
message_get_fd (stream_t stream, int *pfd)
{
  message_t msg;
  body_t body;
  stream_t is;

  if (stream == NULL || (msg = stream->owner) == NULL)
    return EINVAL;

  /* Probably being lazy, then create a body for the stream */
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
  return stream_get_fd (is, pfd);
}

static int
extract_addr (const char *s, size_t n, char **presult, size_t *pnwrite)
{
  char *p, *p1, *p2;

  if (s == NULL || n == 0 || presult == NULL)
    return EINVAL;

  /* skip the double quotes */
  p = memchr (s, '\"', n);
  if (p != NULL)
    {
      p1 = memchr (s, '<', p - s);
      p2 = memchr (s, '@', p - s);
      if (p1 == NULL && p2 == NULL)
        {
          p1 = memchr (p + 1, '\"', n - ((p + 1) - s));
          if (p1 != NULL)
            {
              n -= (p1 + 1) - s;
              s = p1 + 1;
            }
        }
    }

  /*  <name@hostname> ?? */
  p = memchr (s, '<', n);
  if (p != NULL)
    {
      p1 = memchr (p, '>', n - (p - s));
      if (p1 != NULL && (p1 - p) > 1)
        {
          p2 = memchr (p, ' ', p1 - p);
          if (p2 == NULL)
            {
              /* the NULL is already accounted for */
              *presult = calloc (1, p1 - p);
              if (*presult == NULL)
                return ENOMEM;
              memcpy (*presult, p + 1, (p1 - p) - 1);
	      if (pnwrite)
		*pnwrite = (p1 - p) - 1;
              return 0;
            }
        }
    }
  /* name@domain */
  p = memchr (s, '@', n);
  if (p != NULL)
    {
      p1 = p;
      while (*p != ' ' && p != s)
        p--;
      while (*p1 != ' ' && p1 < (s + n))
        p1++;
      *presult = calloc (1, (p1 - p) + 1);
      if (*presult == NULL)
        return ENOMEM;
      memcpy (*presult, p, p1 - p);
      if (pnwrite)
	*pnwrite = p1 - p;
      return 0;
    }

  *presult = NULL;
  return EINVAL;

}
