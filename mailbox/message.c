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


#include <header0.h>
#include <attribute0.h>
#include <message0.h>
#include <mailbox0.h>
#include <io0.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

static int message_read (istream_t is, char *buf, size_t buflen,
			 off_t off, ssize_t *pnread );
static int message_write (ostream_t os, const char *buf, size_t buflen,
			  off_t off, ssize_t *pnwrite);

int message_clone (message_t omsg, message_t *pmsg)
{
  int status;
  FILE *file;
  message_t msg;
  istream_t is;
  ostream_t os;
  header_t header;
  char buffer[BUFSIZ];
  char *pbuf = NULL;
  char *tbuf = NULL;
  off_t offset = 0;
  size_t nread = 0;

  /* retreive the header */
  {
    status = message_get_header (omsg, &header);
    if (status != 0)
      return status;
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


    status = header_init (&header, pbuf, offset, MU_HEADER_RFC822, NULL);
    if (status != 0)
      {
	free (pbuf);
	return status;
      }
    free (pbuf);

  }

  /* retrieve the body */
  {
    file = tmpfile ();
    if (file == NULL)
      {
	header_destroy (&header, NULL);
	return errno;
      }
    offset = 0;
    message_get_istream (omsg, &is);
    do {
      istream_read (is, buffer, sizeof (buffer), offset, &nread);
      fwrite (buffer, sizeof (*buffer), nread, file);
      offset += nread;
    } while (nread > 0);
    rewind (file);
  }

  /* create the message */
  status = message_init (&msg, NULL);
  if (status != 0)
    return status;

  /* set the header */
  message_set_header (msg, header, NULL);

  /* set the body with the streams */
  msg->body = calloc (1, sizeof (*(msg->body)));
  if (msg->body == NULL)
    {
      fclose (file);
      message_destroy (&msg, NULL);
      return ENOMEM;
    }
  status = istream_init (&is, message_read, NULL);
  if (status != 0)
    {
      message_destroy (&msg, NULL);
      return status;
    }
  status = ostream_init (&os, message_write, NULL);
  if (status != 0)
    {
      message_destroy (&msg, NULL);
      return status;
    }
  *pmsg = msg;
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

      msg->ref_count--;
      if ((msg->owner && msg->owner == owner) ||
	  (msg->owner == NULL && msg->ref_count <= 0))
	{
	  /* header */
	  header_destroy (&(msg->header), owner);
	  /* attribute */
	  attribute_destroy (&(msg->attribute), owner);
	  /* istream */
	  istream_destroy (&(msg->is), owner);
	  /* ostream */
	  ostream_destroy (&(msg->os), owner);

	  /* if sometype of floating/temporary message */
	  if (msg->body)
	    {
	      body_t body = msg->body;
	      if (body->file)
		fclose (body->file);
	      free (body->content);
	      free (msg->body);
	    }
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
  msg->header = hdr;
  return 0;
}

int
message_get_istream (message_t msg, istream_t *pis)
{
  if (msg == NULL || pis == NULL)
    return EINVAL;
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
  msg->is = is;
  return 0;
}

int
message_get_ostream (message_t msg, ostream_t *pos)
{
  if (msg == NULL || pos == NULL)
    return EINVAL;
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
  *pattribute = msg->attribute;
  return 0;
}

int
message_set_attribute  (message_t msg, attribute_t attribute, void *owner)
{
  if (msg == NULL)
   return EINVAL;
  if (msg->owner != owner)
    return EACCES;
  msg->attribute = attribute;
  return 0;
}

static int
message_read (istream_t is, char *buf, size_t buflen,
	      off_t off, ssize_t *pnread )
{
  message_t msg = NULL;
  ssize_t nread = 0;

  if (is == NULL)
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
      else if (body->content)
	{
	  off_t ln = body->content_len  - off;
	  if (ln > 0)
	    {
	      nread = ((size_t)ln < buflen) ? ln : buflen;
	      memcpy (buf, body->content + off, nread);
	    }
	  else
	    nread = 0;
	  errno = 0;
	}
      else
	{
	  return EINVAL;
	}
    }

  if (pnread)
    *pnread = nread;
  return 0;
}

static int
message_write (ostream_t os, const char *buf, size_t buflen,
	       off_t off, ssize_t *pnwrite)
{
  message_t msg = NULL;
  ssize_t nwrite = 0;

  if (os == NULL)
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
      else if (body->content)
	{
	  off_t ln = body->content_len  - off;
	  if (ln > 0)
	    {
	      nwrite = ((size_t)ln < buflen) ? ln : buflen;
	      memcpy (body->content + off, buf, nwrite);
	    }
	  else
	    nwrite = 0;
	}
      else
	{
	  return EINVAL;
	}
    }

  if (pnwrite)
    *pnwrite = nwrite;
  return 0;
}
