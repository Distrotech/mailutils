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


#include <header.h>
#include <attribute.h>
#include <message0.h>
#include <mailbox0.h>
#include <io0.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

static int body_init (body_t *pbody, void *owner);
static void body_destroy (body_t *pbody, void *owner);
static int message_read (stream_t is, char *buf, size_t buflen,
			 off_t off, size_t *pnread );
static int message_write (stream_t os, const char *buf, size_t buflen,
			  off_t off, size_t *pnwrite);
static int message_get_fd (stream_t stream, int *pfd);

int
message_clone (message_t msg)
{
  int status;
  stream_t stream;
  header_t header;
  attribute_t attribute;
  body_t body;
  char buffer[BUFSIZ];
  char *pbuf = NULL;
  char *tbuf = NULL;
  off_t offset = 0;
  size_t nread = 0;

  if (msg == NULL)
    return EINVAL;

  /* If is not own, then it's a floating message
   * just bump the reference count.
   */
  if (msg->owner == NULL)
    {
      if (msg->ref_count <= 0)
	msg->ref_count = 1;
      else
	msg->ref_count++;
      return 0;
    }

  /* retreive the header */
  {
    header = msg->header;
    status = header_get_stream (header, &stream);
    if (status != 0)
      return status;

    do {
      status = stream_read (stream, buffer, sizeof (buffer), offset, &nread);
      if (status != 0)
	{
	  free (pbuf);
	  return status;
	}
      if (nread == 0)
	break;
      tbuf = realloc (pbuf, offset + nread);
      if (tbuf == NULL)
	{
	  free (pbuf);
	  return ENOMEM;
	}
      else
	pbuf = tbuf;
      memcpy (pbuf + offset, buffer, nread);
      offset += nread;
    } while (nread > 0);

  }
  /* set the new header */
  status = header_init (&header, pbuf, offset, msg);
  if (status != 0)
    {
      free (pbuf);
      return status;
    }
  free (pbuf);

  /* retrieve the body */
  {
    status = body_init (&body, msg);
    if (status != 0)
      {
	header_destroy (&header, msg);
	return status;
      }

    stream = msg->stream;
    offset = 0;
    do {
      do
	{
	  status = stream_read (stream, buffer, sizeof (buffer), offset, &nread);
	} while (status == EAGAIN);
      if (status != 0)
	{
	  header_destroy (&header, msg);
	  body_destroy (&body, msg);
	  return status;
	}
      fwrite (buffer, sizeof (*buffer), nread, body->file);
      offset += nread;
    } while (nread > 0);
    rewind (body->file);
  }

  /* set the body with the streams */
  status = stream_init (&stream, msg);
  if (status != 0 )
    {
      header_destroy (&header, msg);
      body_destroy (&body, msg);
      return status;
    }
  stream_set_read (stream, message_read, msg);
  stream_set_write (stream, message_write, msg);

  /* attribute */
  status = attribute_init (&attribute, msg);
  if (status != 0)
    {
      header_destroy (&header, msg);
      body_destroy (&body, msg);
      stream_destroy (&stream, msg);
    }
  attribute_copy (attribute, msg->attribute, msg);

  /* every thing went ok */
  msg->header = header;
  msg->attribute =  attribute;
  msg->stream = stream;
  msg->body = body;
  msg->size = offset;
  msg->ref_count++;
  msg->owner = NULL; /* orphan */
  return 0;
}

int
message_init (message_t *pmsg, void *owner)
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
      int destroy = 0;

      /* always decremente */
      msg->ref_count--;

      if (msg->owner && msg->owner == owner)
	destroy = 1;

      if (msg->owner == NULL && msg->ref_count <= 0)
	{
	  destroy = 1;
	  owner = msg;
	}

      if (destroy)
	{
	  header_t header = msg->header;
	  attribute_t attribute = msg->attribute;
	  stream_t stream = msg->stream;
	  body_t body = msg->body;

	  /* notify the listeners */
	  message_notification (msg, MU_EVT_MSG_DESTROY);
	  /* header */
	  header_destroy (&header, owner);
	  /* attribute */
	  attribute_destroy (&attribute, owner);
	  /* stream */
	  stream_destroy (&stream, owner);

	  /* if sometype of floating/temporary message */
	  if (body)
	    {
	      if (body->file)
		fclose (body->file);
	      free (body);
	    }
	  /* notifications are done */
	  free (msg->event);

	  /* check again for resurrection before free()ing
	   * the memory maybe it was clone, if yes we can not
	   * free the pointer.
	   *
	   */
	  if (msg->ref_count <= 0)
	    free (msg);
	}
      /* loose the link */
      *pmsg = NULL;
    }
}

int
message_get_header (message_t msg, header_t *phdr)
{
  if (phdr == NULL || msg == NULL)
    return EINVAL;

  /* is it a floating mesg */
  if (msg->header == NULL && msg->owner == NULL)
    {
      header_t header;
      int status = header_init (&header, NULL, 0, msg);
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
message_get_stream (message_t msg, stream_t *pstream)
{
  if (msg == NULL || pstream == NULL)
    return EINVAL;

  if (msg->stream == NULL && msg->owner == NULL)
    {
      stream_t stream;
      int status;
      /* lazy floating message the body is created when
       * doing the first message_write creation
       */
      status = stream_init (&stream, msg);
      if (status != 0)
	return status;
      stream_set_read (stream, message_read, msg);
      stream_set_write (stream, message_write, msg);
      stream_set_fd (stream, message_get_fd, msg);
      /* make sure we've clean */
      stream_destroy (&(msg->stream), msg);
      msg->stream = stream;
    }

  *pstream = msg->stream;
  return 0;
}

int
message_set_stream (message_t msg, stream_t stream, void *owner)
{
  if (msg == NULL)
   return EINVAL;
  if (msg->owner != owner)
    return EACCES;
  /* make sure we destroy the old one if it is own by the message */
  stream_destroy (&(msg->stream), msg);
  msg->stream = stream;
  return 0;
}

int
message_get_size (message_t msg, size_t *psize)
{
  if (msg == NULL)
    return EINVAL;
  if (psize)
    *psize = msg->size;
  return 0;
}

int
message_set_size (message_t msg, size_t size, void *owner)
{
  if (msg == NULL)
    return EINVAL;
  if (msg->owner != owner)
    return EACCES;
  msg->size = size;
  return 0;
}

int
message_get_attribute (message_t msg, attribute_t *pattribute)
{
  if (msg == NULL || pattribute == NULL)
   return EINVAL;
  if (msg->attribute == NULL && msg->owner == NULL)
    {
      attribute_t attribute;
      int status = attribute_init (&attribute, msg);
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
  attribute_destroy (&(msg->attribute), msg);
  msg->attribute = attribute;
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
      if (event->_action == action)
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

/* We use the notion of body_t here */
static int
body_init (body_t *pbody, void *owner)
{
  body_t body;
  FILE *file;

  if (pbody == NULL)
    return EINVAL;

#ifdef HAVE_MKSTEMP
  {
    char tmpbuf[L_tmpnam + 1];
    int fd;

    if (tmpnam (tmpbuf) == NULL ||
	(fd = mkstemp (tmpbuf)) == -1 ||
	(file = fdopen(fd, "w+")) == NULL)
      return errno;
    (void)remove(tmpbuf);
  }
#else
  file = tmpfile ();
  if (file == NULL)
    return errno;
  /* make sure the mode is right */
  fchmod (fileno (file), 0600);
#endif

  /* set the body with the streams */
  body = calloc (1, sizeof (*body));
  if (body == NULL)
    {
      fclose (file);
      return ENOMEM;
    }
  body->file = file;
  body->owner = owner;
  *pbody = body;
  return 0;
}

static void
body_destroy (body_t *pbody, void *owner)
{
  if (pbody && *pbody)
    {
      body_t body = *pbody;
      if (body->owner == owner)
	{
	  if (body->file)
	    fclose (body->file);
	}
      *pbody = NULL;
    }
}

static int
message_read (stream_t is, char *buf, size_t buflen,
	      off_t off, size_t *pnread )
{
  message_t msg;
  size_t nread = 0;

  if (is == NULL || (msg = is->owner) == NULL)
    return EINVAL;

  if (msg->body)
    {
      body_t body = msg->body;
      if (body->file)
	{
	  /* we're not checking the error of fseek for some handlers
	   * like socket in those not make sense.
	   * FIXME: Alternative is to check fseeck and errno == EBADF
	   * if not a seekable stream.
	   */
	  fseek (body->file, off, SEEK_SET);
	  nread = fread (buf, sizeof (char), buflen, body->file);
	  if (nread == 0)
	    {
	      if (ferror (body->file))
		return errno;
	      /* clear the error for feof() */
	      clearerr (body->file);
	    }
	  /* errno set by fread()/fseek() ? */
	}
      else
	return EINVAL;
    }

  if (pnread)
    *pnread = nread;
  return 0;
}

static int
message_write (stream_t os, const char *buf, size_t buflen,
	       off_t off, size_t *pnwrite)
{
  message_t msg;
  size_t nwrite = 0;
  body_t body;

  if (os == NULL || (msg = os->owner) == NULL)
    return EINVAL;

  /* Probably being lazy, then create a body for the stream */
  if (msg->body == NULL)
    {
      int status = body_init (&body, msg);
      if (status != 0 )
        return status;
      msg->body = body;
    }
  else
      body = msg->body;

  if (body->file)
    {
      /* we're not checking the error of fseek for some handlers
       * like socket in those not make sense.
       * FIXME: Alternative is to check fseeck and errno == EBADF
       * if not a seekable stream.
       */
      fseek (body->file, off, SEEK_SET);
      nwrite = fwrite (buf, sizeof (char), buflen, body->file);
      if (nwrite == 0)
	{
	  if (ferror (body->file))
	    return errno;
	  /* clear the error for feof() */
	  clearerr (body->file);
	}
      /* errno set by fread()/fseek() ? */
    }
  else
    return EINVAL;

  if (pnwrite)
    *pnwrite = nwrite;
  return 0;
}

static int
message_get_fd (stream_t stream, int *pfd)
{
  message_t msg;
  body_t body;

  if (stream == NULL || (msg = stream->owner) == NULL)
    return EINVAL;

  /* Probably being lazy, then create a body for the stream */
  if (msg->body == NULL)
    {
      int status = body_init (&body, msg);
      if (status != 0 )
        return status;
      msg->body = body;
    }
  else
      body = msg->body;

  if (pfd)
   *pfd = fileno (body->file);

  return 0;
}
