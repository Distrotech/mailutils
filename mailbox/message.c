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

int
message_init (message_t *pmsg)
{
  message_t msg;

  if (pmsg == NULL)
    return EINVAL;
  msg = calloc (1, sizeof (*msg));
  if (msg == NULL)
    return ENOMEM;
  *pmsg = msg;
  return 0;
}

void
message_destroy (message_t *pmsg)
{
  if (pmsg && *pmsg)
    {
      message_t msg = *pmsg;
      /*
       * The message has a mailbox owner
       * let the mailbox destroy when it finishes
       */
      if (msg->mailbox)
	{
	  /* but still loose the link */
	  *pmsg = NULL;
	  return;
	}
      /* is the header own by us ? */
      if (msg->header && msg->header->message == msg)
	{
	  msg->header->message = NULL;
	  header_destroy (&(msg->header));
	}
      /* is the attribute own by us ? */
      if (msg->attribute && msg->attribute->message == msg)
	{
	  msg->attribute->message = NULL;
	  attribute_destroy (&(msg->attribute));
	}
      /* is the istream own by us */
      if (msg->is && msg->is->owner == msg)
	{
	  msg->is->owner = NULL;
	  istream_destroy (&(msg->is));
	}
      /* is the ostream own by us */
      if (msg->os && msg->os->owner == msg)
	{
	  msg->os->owner = NULL;
	  ostream_destroy (&(msg->os));
	}
      /* is it sometype of floating/temporary message */
      if (msg->body)
	{
	  body_t body = msg->body;
	  if (body->file)
	    fclose (body->file);
	  free (body->content);
	  free (msg->body);
	}
      free (msg);
      /* loose the link */
      *pmsg = NULL;
    }
}

int
message_get_header (message_t msg, header_t *phdr)
{
  int err = 0;
  int nread;
  char *tbuf = NULL;
  char buf[BUFSIZ];

  if (phdr == NULL || msg == NULL)
    return EINVAL;

  /* header allready retrieve ? */
  if (msg->header != NULL)
    {
      *phdr = msg->header;
      return 0;
    }

  if (msg->mailbox == NULL)
    return EINVAL;

  /* Ok this is where the fun begins, we have to take to account
   * the F&$#*#^#&g O_NONBLOCKING, thanks very much to Brian and David.
   * So POP for example is a stream protocol where the server
   * keeps on sending the data til' the wire turn red. So when
   * we hit a bottleneck (EAGAIN, EWOULDBLOCK, EINPROGRESS, etc ..)
   * we save our state and propagate the error up.  Hopefully
   * the people upstairs will do the right thing by recalling
   * us again.
   * To resume my thoughts ... I do not like it.
   */
  while ((nread = mailbox_get_header (msg->mailbox,
				      msg->num, buf, sizeof(buf),
				      msg->header_offset, &err)) > 0)
    {
      tbuf = realloc (msg->header_buffer, msg->header_offset + nread);
      if (tbuf == NULL)
	{
	  free (msg->header_buffer);
	  msg->header_buffer = NULL;
	  msg->header_offset = 0;
	  return ENOMEM;
	}
      else
	msg->header_buffer = tbuf;
      memcpy (msg->header_buffer + msg->header_offset, buf, nread);
      msg->header_offset += nread;
    }

  if (nread < 0)
    {
      if (err == EAGAIN || err == EINPROGRESS || err == EWOULDBLOCK)
	return EAGAIN;
      free (msg->header_buffer);
      msg->header_buffer = NULL;
      msg->header_offset = 0;
      return err;
    }

  err = header_init (&(msg->header), msg->header_buffer,
		     msg->header_offset, MU_HEADER_RFC822);
  if (err == 0)
    {
      /* we own it */
      msg->header->message = msg;
      *phdr = msg->header;
      msg->header = NULL;
    }
  /* we can discard it */
  free (msg->header_buffer);
  msg->header_buffer = NULL;
  msg->header_offset = 0;
  return err;
}

int
message_set_header (message_t msg, header_t hdr)
{
  /* Can not do that on a mailbox */
  if (msg == NULL || msg->mailbox == NULL)
    return EINVAL;
  /* we own it ? */
  if (msg->header && msg->header->message == msg)
    {
      msg->header->message = NULL;
      header_destroy (&(msg->header));
    }
  msg->header = hdr;
  return 0;
}

static ssize_t
message_read (istream_t is, char *buf, size_t buflen, off_t off)
{
  message_t msg = NULL;
  ssize_t nread = -1;
  int err = 0;

  if (is == NULL || (msg = (message_t)is->owner) == NULL)
    {
      errno = EINVAL;
      return -1;
    }

  /* is it own by a mailbox ? */
  if (msg->mailbox)
    {
      nread = mailbox_get_body (msg->mailbox, msg->num, buf,
				buflen, off, &err);
      errno = err;
    }
  else if (msg->body)
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
		nread = -1;
	      /* clear the error even for feof() */
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
	  errno = EINVAL;
	  nread = -1;
	}
    }

  return nread;
}

int
message_get_istream (message_t msg, istream_t *pis)
{
  int err;
  if (msg == NULL || pis == NULL)
    return EINVAL;
  /* already done */
  if  (msg->is)
    *pis = msg->is;

  err = istream_init (&(msg->is));
  if (err != 0)
    return err;
  /* tell the world this is ours */
  msg->is->owner = msg;
  msg->is->_read = message_read;
  *pis = msg->is;
  return 0;
}

static int
message_write (ostream_t os, const char *buf, size_t buflen, off_t off)
{
  message_t msg = NULL;
  ssize_t nwrite = -1;

  if (os == NULL || (msg = (message_t)os->owner) == NULL)
    {
      errno = EINVAL;
      return -1;
    }

  /* is it own by a mailbox ? */
  if (msg->mailbox)
    {
      /* We can not write in a mailbox this way */
      errno = ENOSYS;
      return -1;
    }
  else if (msg->body)
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
		nwrite = -1;
	      /* clear the error even for feof() */
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
	  errno = 0;
	}
      else
	{
	  errno = EINVAL;
	  nwrite = -1;
	}
    }

  return nwrite;
}

int
message_get_ostream (message_t msg, ostream_t *pos)
{
  int err;
  if (msg == NULL || pos == NULL)
    return EINVAL;
  /* already done */
  if  (msg->os)
    *pos = msg->os;

  err = ostream_init (&(msg->os));
  if (err != 0)
    return err;
  /* tell the world this is ours */
  msg->os->owner = msg;
  msg->os->_write = message_write;
  *pos = msg->os;
  return 0;
}

int
message_size (message_t msg, size_t *size)
{
  if (msg == NULL || size == NULL)
   return EINVAL;
  if (msg->mailbox)
    {
      size_t hs, bs;
      int status;
      status = mailbox_get_size (msg->mailbox, msg->num, &hs, &bs);
      if (status != 0)
	return status;
      *size = hs + bs;
      return 0;
    }
  return ENOSYS;
}

int
message_get_attribute (message_t msg, attribute_t *pattribute)
{
  int status;

  if (msg == NULL || pattribute == NULL)
   return EINVAL;

  /* killroy was here ? */
  if (msg->attribute)
    *pattribute = msg->attribute;

  if (msg->mailbox)
    {
      int status;
      status = mailbox_get_attribute (msg->mailbox, msg->num, pattribute);
      if (status != 0)
	return status;
      msg->attribute = *pattribute;
      /* set the owner of the attribute to be us */
      (*pattribute)->message = msg;
      return 0;
    }

  status = attribute_init (&(msg->attribute));
  if (status == 0)
    {
      /* we own this baby */
      msg->attribute->message = msg;
      *pattribute = msg->attribute;
    }

  return status;
}

int
message_set_attribute  (message_t msg, attribute_t attribute)
{
  if (msg == NULL)
   return EINVAL;

  /* own by a mailbox can no set attribute this way */
  if (msg->mailbox)
    return ENOSYS;

  /* we own it ? */
  if (msg->attribute && msg->attribute->message == msg)
    {
      /* orphan it */
      msg->attribute->message = NULL;
      attribute_destroy (&(msg->attribute));
    }
  msg->attribute = attribute;
  return 0;
}
