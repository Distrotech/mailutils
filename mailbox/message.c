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

static int body_init (body_t *pbody, void *owner);
static void body_destroy (body_t *pbody, void *owner);
static int message_read (istream_t is, char *buf, size_t buflen,
			 off_t off, ssize_t *pnread );
static int message_write (ostream_t os, const char *buf, size_t buflen,
			  off_t off, ssize_t *pnwrite);

int
message_clone (message_t msg)
{
  int status;
  istream_t is;
  ostream_t os;
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
    status = header_get_istream (header, &is);
    if (status != 0)
      return status;

    do {
      status = istream_read (is, buffer, sizeof (buffer), offset, &nread);
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

    is = msg->is;
    offset = 0;
    do {
      do
	{
	  status = istream_read (is, buffer, sizeof (buffer), offset, &nread);
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
  status = istream_init (&is, message_read, msg);
  if (status != 0)
    {
      header_destroy (&header, msg);
      body_destroy (&body, msg);
      return status;
    }
  status = ostream_init (&os, message_write, msg);
  if (status != 0)
    {
      header_destroy (&header, msg);
      body_destroy (&body, msg);
      istream_destroy (&is, msg);
      return status;
    }

  /* attribute */
  status = attribute_init (&attribute, msg);
  if (status != 0)
    {
      header_destroy (&header, msg);
      body_destroy (&body, msg);
      istream_destroy (&is, msg);
      ostream_destroy (&os, msg);
    }
  attribute_copy (attribute, msg->attribute);

  /* every thing went ok */
  msg->header = header;
  msg->attribute =  attribute;
  msg->is = is;
  msg->os = os;
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
	  istream_t is = msg->is;
	  ostream_t os = msg->os;
	  body_t body = msg->body;

	  /* notify the listeners */
	  message_notification (msg, MU_EVT_MSG_DESTROY);
	  /* header */
	  header_destroy (&header, owner);
	  /* attribute */
	  attribute_destroy (&attribute, owner);
	  /* istream */
	  istream_destroy (&is, owner);
	  /* ostream */
	  ostream_destroy (&os, owner);

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
message_get_istream (message_t msg, istream_t *pis)
{
  if (msg == NULL || pis == NULL)
    return EINVAL;

  /* lazy floating message body creation */
  if (msg->is == NULL && msg->owner == NULL)
    {
      body_t body;
      istream_t is;
      ostream_t os;
      int status;
      status = body_init (&body, msg);
      if (status != 0 )
	return status;
      status = istream_init (&is, message_read, msg);
      if (status != 0)
	{
	  body_destroy (&body, msg);
	  return status;
	}
      status = ostream_init (&os, message_write, msg);
      if (status != 0)
	{
	  istream_destroy (&is, msg);
	  body_destroy (&body, msg);
	  return status;
	}
      /* make sure we've clean */
      istream_destroy (&(msg->is), msg);
      ostream_destroy (&(msg->os), msg);
      body_destroy (&(msg->body), msg);
      msg->is = is;
      msg->os = os;
      msg->body = body;
    }

  *pis = msg->is;
  return 0;
}

int
message_set_istream (message_t msg, istream_t is, void *owner)
{
  if (msg == NULL)
   return EINVAL;
  if (msg->owner != owner)
    return EACCES;
  /* make sure we destroy the old one if it is own by the message */
  istream_destroy (&(msg->is), msg);
  msg->is = is;
  return 0;
}

int
message_get_ostream (message_t msg, ostream_t *pos)
{
  if (msg == NULL || pos == NULL)
    return EINVAL;

  /* lazy floating message body creation */
  if (msg->os == NULL && msg->owner == NULL)
    {
      body_t body;
      istream_t is;
      ostream_t os;
      int status;
      status = body_init (&body, msg->owner);
      if (status != 0 )
	return status;
      status = istream_init (&is, message_read, msg);
      if (status != 0)
	{
	  body_destroy (&body, msg->owner);
	  return status;
	}
      status = ostream_init (&os, message_write, msg);
      if (status != 0)
	{
	  istream_destroy (&is, msg);
	  body_destroy (&body, msg);
	  return status;
	}
      /* make sure we've clean */
      istream_destroy (&(msg->is), msg);
      ostream_destroy (&(msg->os), msg);
      body_destroy (&(msg->body), msg);
      msg->is = is;
      msg->os = os;
      msg->body = body;
    }

  *pos = msg->os;
  return 0;
}

int
message_set_ostream (message_t msg, ostream_t os, void *owner)
{
  if (msg == NULL)
   return EINVAL;
  if (msg->owner != owner)
    return EACCES;
  /* make sure we destroy the old one if it is own by the message */
  ostream_destroy (&(msg->os), msg);
  msg->os = os;
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
message_read (istream_t is, char *buf, size_t buflen,
	      off_t off, ssize_t *pnread )
{
  message_t msg;
  ssize_t nread = 0;

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
message_write (ostream_t os, const char *buf, size_t buflen,
	       off_t off, ssize_t *pnwrite)
{
  message_t msg;
  ssize_t nwrite = 0;

  if (os == NULL || (msg = os->owner) == NULL)
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
    }

  if (pnwrite)
    *pnwrite = nwrite;
  return 0;
}
