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
       * The message has an mailbox owner
       * let the mailbox destroy when it finishes
       */
      if (msg->mailbox)
	{
	  *pmsg = NULL;
	  return;
	}
      /* is the header own by us ? */
      if (msg->header->message == msg)
	msg->header->message = NULL;
      header_destroy (&(msg->header));
      /* is the attribute own by us ? */
      if (msg->attribute->message == msg)
	msg->attribute->message = NULL;
      attribute_destroy (&(msg->attribute));
      if (msg->body)
	{
	  body_t body = msg->body;
	  if (body->file)
	    fclose (body->file);
	  free (body->content);
	}
      free (msg->body);
      if (msg->part_num > 0)
	{
	  size_t i;
	  for (i = 0; i < msg->part_num; i++)
	    message_destroy (&(msg->parts[i]));
	}
      free (msg->parts);
      free (msg);
      *pmsg = NULL;
    }
}

int
message_get_header (message_t msg, header_t *phdr)
{
  int err = 0;
  int nread, n = 0;
  char *pbuf = NULL, *tbuf = NULL;
  char buf [BUFSIZ];
  int off = 0;

  if (phdr == NULL || msg == NULL)
    return EINVAL;

  if (msg->header != NULL)
    {
      *phdr = msg->header;
      return 0;
    }

  if (msg->mailbox == NULL)
    return EINVAL;

  do
    {
      nread = mailbox_get_header (msg->mailbox, msg->num, buf, sizeof(buf),
				off, &err);
      if (err == EAGAIN || err == EINPROGRESS)
	continue;
      else if (err != 0)
	{
	  free (pbuf);
	  return err;
	}
      off += nread;
      tbuf = realloc (pbuf, off);
      if (tbuf == NULL)
	{
	  free (pbuf);
	  return ENOMEM;
	}
      else
	pbuf = tbuf;
      memcpy (pbuf + n, buf, nread);
      n = nread;
    } while (nread > 0);

  err = header_init (&(msg->header), pbuf, off, MU_HEADER_RFC822);
  if (err == 0)
    {
      /* we own it */
      msg->header->message = msg;
      *phdr = msg->header;
    }
  free (pbuf);
  return err;
}

int
message_set_header (message_t msg, header_t hdr)
{
  if (msg == NULL || msg->mailbox == NULL)
    return EINVAL;
  if (msg->header)
    {
      if (msg->header->message == msg)
	msg->header->message = NULL;
      header_destroy (&(msg->header));
    }
  msg->header = hdr;
  return 0;
}

ssize_t
message_get_content (message_t msg, char *buf, size_t buflen,
		     off_t off, int *err)
{
  if (msg == NULL || msg->mailbox == NULL)
    {
      if (err)
	*err = EINVAL;
      return -1;
    }
  return mailbox_get_body (msg->mailbox, msg->num, buf, buflen, off, err);
}

int
message_set_content (message_t msg, char *buf, size_t buflen)
{
  (void)msg;(void)buf; (void)buflen;
  return ENOSYS;
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
      if (status == 0)
	{
	  if (size)
	    *size = hs + bs;
	}
      return status;
    }
  return ENOSYS;
}
