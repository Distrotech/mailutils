/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2004 Free Software Foundation, Inc.

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

#ifdef ENABLE_NNTP

#include <termios.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>

#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif

#include <md5.h>

#include <mailutils/body.h>
#include <mailutils/debug.h>
#include <mailutils/errno.h>
#include <mailutils/error.h>
#include <mailutils/header.h>
#include <mailutils/message.h>
#include <mailutils/observer.h>
#include <mailutils/property.h>
#include <mailutils/stream.h>
#include <mailutils/iterator.h>
#include <mailutils/url.h>
#include <mailutils/nntp.h>

#include <folder0.h>
#include <mailbox0.h>
#include "nntp0.h"


/*  Functions/Methods that implements the mailbox_t API.  */
static void nntp_mailbox_destroy         __P ((mailbox_t));
static int  nntp_mailbox_open            __P ((mailbox_t, int));
static int  nntp_mailbox_close           __P ((mailbox_t));
static int  nntp_mailbox_get_message     __P ((mailbox_t, size_t, message_t *));
static int  nntp_mailbox_messages_count  __P ((mailbox_t, size_t *));
static int  nntp_mailbox_scan            __P ((mailbox_t, size_t, size_t *));
/* FIXME
   static int  nntp_mailbox_get_size        __P ((mailbox_t, off_t *)); */

static int  nntp_message_get_transport2  __P ((stream_t, mu_transport_t *, mu_transport_t *));
static int  nntp_message_read            __P ((stream_t, char *, size_t, off_t, size_t *));
static int  nntp_message_size            __P ((message_t, size_t *));
/* FIXME
   static int  nntp_message_line            __P ((message_t, size_t *)); */
static int  nntp_message_uidl            __P ((message_t, char *, size_t, size_t *));
static int  nntp_message_uid             __P ((message_t, size_t *));

/* FIXME
   static int  nntp_header_get_transport2   __P ((header_t, char *,
                                                  size_t, off_t, size_t *)); */
static int  nntp_header_fill             __P ((header_t, char *, size_t, off_t, size_t *));

static int  nntp_body_get_transport2     __P ((stream_t, mu_transport_t *, mu_transport_t *));
static int  nntp_body_read               __P ((stream_t, char *, size_t, off_t, size_t *));
static int  nntp_body_size               __P ((body_t, size_t *));
static int  nntp_body_lines              __P ((body_t, size_t *));

static int  nntp_get_transport2          __P ((msg_nntp_t, mu_transport_t *, mu_transport_t *));

int
_nntp_mailbox_init (mailbox_t mbox)
{
  m_nntp_t m_nntp;
  int status = 0;
  size_t name_len = 0;

  /* Allocate specifics for nntp data.  */
  m_nntp = mbox->data = calloc (1, sizeof (*m_nntp));
  if (mbox->data == NULL)
    return ENOMEM;

  /* Get the back pointer of the concrete folder. */
  if (mbox->folder)
    m_nntp->f_nntp = mbox->folder->data;

  m_nntp->mailbox = mbox;		/* Back pointer.  */

  /* Retrieve the name of the newsgroup from the URL.  */
  url_get_path (mbox->url, NULL, 0, &name_len);
  if (name_len == 0)
    {
      /* name "INBOX" is the default.  */
      m_nntp->name = calloc (6, sizeof (char));
      strcpy (m_nntp->name, "INBOX");
    }
  else
    {
      char *p;
      m_nntp->name = calloc (name_len + 1, sizeof (char));
      url_get_path (mbox->url, m_nntp->name, name_len + 1, NULL);
      p = strchr (m_nntp->name,'/');
      if (p)
	*p = '\0';
    }

  /* Initialize the structure.  */
  mbox->_destroy = nntp_mailbox_destroy;

  mbox->_open = nntp_mailbox_open;
  mbox->_close = nntp_mailbox_close;

  /* Messages.  */
  mbox->_get_message = nntp_mailbox_get_message;
  mbox->_messages_count = nntp_mailbox_messages_count;
  mbox->_messages_recent = nntp_mailbox_messages_count;
  mbox->_message_unseen = nntp_mailbox_messages_count;
  /*mbox->_expunge = nntp_mailbox_expunge;*/

  mbox->_scan = nntp_mailbox_scan;
  /*mbox->_is_updated = nntp_mailbox_is_updated; */

  /*mbox->_get_size = nntp_mailbox_get_size; */

  /* Set our properties.  */
  {
    property_t property = NULL;
    mailbox_get_property (mbox, &property);
    property_set_value (property, "TYPE", "NNTP", 1);
  }

  return status;
}

/*  Cleaning up all the ressources associate with a newsgroup/mailbox.  */
static void
nntp_mailbox_destroy (mailbox_t mbox)
{
  if (mbox->data)
    {
      m_nntp_t m_nntp = mbox->data;
      f_nntp_t f_nntp = m_nntp->f_nntp;
      size_t i;

      /* Deselect.  */
      if (m_nntp == f_nntp->selected)
	f_nntp->selected = NULL;

      monitor_wrlock (mbox->monitor);

      if (m_nntp->name)
	free (m_nntp->name);

      /* Destroy the nntp messages and ressources associated to them.  */
      for (i = 0; i < m_nntp->messages_count; i++)
	{
	  if (m_nntp->messages[i])
	    {
	      message_destroy (&(m_nntp->messages[i]->message), m_nntp->messages[i]);
	      if (m_nntp->messages[i]->mid)
		free (m_nntp->messages[i]->mid);
	      free (m_nntp->messages[i]);
	      m_nntp->messages[i] = NULL;
	    }
	}
      if (m_nntp->messages)
	free (m_nntp->messages);
      free (m_nntp);
      mbox->data = NULL;
      monitor_unlock (mbox->monitor);
    }
}

/* If the connection was not up it is open by the folder since the stream
   socket is actually created by the folder.  It is not necessary
   to set select the mailbox/newsgoup right away, there are maybe on going operations.
   But on any operation by a particular mailbox, it will be selected first.  */
static int
nntp_mailbox_open (mailbox_t mbox, int flags)
{
  int status = 0;
  m_nntp_t m_nntp = mbox->data;
  f_nntp_t f_nntp = m_nntp->f_nntp;
  iterator_t iterator;

  /* m_nntp must have been created during mailbox initialization. */
  /* assert (mbox->data);
     assert (m_nntp->name); */

  mbox->flags = flags;

  /* make sure the connection is up.  */
  if ((status = folder_open (f_nntp->folder, flags)))
    return status;

  mu_nntp_set_debug (f_nntp->nntp, mbox->debug);

  /* We might not have to SELECT the newsgroup, but we need to know it
     exists.  */
  status = mu_nntp_list_active (f_nntp->nntp, m_nntp->name, &iterator);
  if (status == 0)
    {
      for (iterator_first (iterator);
           !iterator_is_done (iterator); iterator_next (iterator))
        {
          char *buffer = NULL;
          iterator_current (iterator, (void **) &buffer);
          mu_nntp_parse_list_active (buffer, NULL, &m_nntp->high, &m_nntp->low, &m_nntp->status);
        }
      iterator_destroy (&iterator);
    }
  return status;
}

/* We can not close the folder in term of shuting down the connection but if
   we were the selected mailbox/newsgroup we deselect ourself.  */
static int
nntp_mailbox_close (mailbox_t mailbox)
{
  m_nntp_t m_nntp = mailbox->data;
  f_nntp_t f_nntp = m_nntp->f_nntp;
  int i;

  monitor_wrlock (mailbox->monitor);

  /* Destroy the nntp posts and ressources associated to them.  */
  for (i = 0; i < m_nntp->messages_count; i++)
    {
      if (m_nntp->messages[i])
	{
	  msg_nntp_t msg_nntp = m_nntp->messages[i];
	  if (msg_nntp->message)
	    message_destroy (&(msg_nntp->message), msg_nntp);
	}
      free (m_nntp->messages[i]);
    }
  if (m_nntp->messages)
    free (m_nntp->messages);
  m_nntp->messages = NULL;
  m_nntp->messages_count = 0;
  m_nntp->number = 0;
  m_nntp->low = 0;
  m_nntp->high = 0;
  monitor_unlock (mailbox->monitor);

  /* Deselect.  */
  if (m_nntp != f_nntp->selected)
    f_nntp->selected = NULL;

  /* Decrement the ref count. */
  return folder_close (mailbox->folder);
}

static int
nntp_mailbox_get_message (mailbox_t mbox, size_t msgno, message_t *pmsg)
{
  m_nntp_t m_nntp = mbox->data;
  msg_nntp_t msg_nntp;
  message_t msg = NULL;
  int status;
  size_t i;

  /* Sanity.  */
 if (pmsg == NULL)
    return MU_ERR_OUT_PTR_NULL;

 msgno--;
  monitor_rdlock (mbox->monitor);
  /* See if we have already this message.  */
  for (i = 0; i < m_nntp->messages_count; i++)
    {
      if (m_nntp->messages[i])
	{
	  if (m_nntp->messages[i]->msgno == msgno + m_nntp->low)
	    {
	      *pmsg = m_nntp->messages[i]->message;
	      monitor_unlock (mbox->monitor);
	      return 0;
	    }
	}
    }
  monitor_unlock (mbox->monitor);

  msg_nntp = calloc (1, sizeof (*msg_nntp));
  if (msg_nntp == NULL)
    return ENOMEM;

  /* Back pointer.  */
  msg_nntp->m_nntp = m_nntp;
  msg_nntp->msgno = msgno + m_nntp->low;

  /* Create the message.  */
  {
    stream_t stream = NULL;
    if ((status = message_create (&msg, msg_nntp)) != 0
	|| (status = stream_create (&stream, mbox->flags, msg)) != 0)
      {
	stream_destroy (&stream, msg);
	message_destroy (&msg, msg_nntp);
	free (msg_nntp);
	return status;
      }
    /* Help for the readline()s  */
    stream_set_read (stream, nntp_message_read, msg);
    stream_set_get_transport2 (stream, nntp_message_get_transport2, msg);
    message_set_stream (msg, stream, msg_nntp);
    message_set_size (msg, nntp_message_size, msg_nntp);
  }

  /* Create the header.  */
  {
    header_t header = NULL;
    if ((status = header_create (&header, NULL, 0,  msg)) != 0)
      {
	message_destroy (&msg, msg_nntp);
	free (msg_nntp);
	return status;
      }
    header_set_fill (header, nntp_header_fill, msg);
    message_set_header (msg, header, msg_nntp);
  }

  /* Create the body and its stream.  */
  {
    body_t body = NULL;
    stream_t stream = NULL;
    if ((status = body_create (&body, msg)) != 0
	|| (status = stream_create (&stream, mbox->flags, body)) != 0)
      {
	body_destroy (&body, msg);
	stream_destroy (&stream, body);
	message_destroy (&msg, msg_nntp);
	free (msg_nntp);
	return status;
      }
    /* Helps for the readline()s  */
    stream_set_read (stream, nntp_body_read, body);
    stream_set_get_transport2 (stream, nntp_body_get_transport2, body);
    body_set_size (body, nntp_body_size, msg);
    body_set_lines (body, nntp_body_lines, msg);
    body_set_stream (body, stream, msg);
    message_set_body (msg, body, msg_nntp);
  }

  /* Set the UID on the message. */
  message_set_uid (msg, nntp_message_uid, msg_nntp);

  /* Add it to the list.  */
  monitor_wrlock (mbox->monitor);
  {
    msg_nntp_t *m ;
    m = realloc (m_nntp->messages, (m_nntp->messages_count + 1)*sizeof (*m));
    if (m == NULL)
      {
	message_destroy (&msg, msg_nntp);
	free (msg_nntp);
	monitor_unlock (mbox->monitor);
	return ENOMEM;
      }
    m_nntp->messages = m;
    m_nntp->messages[m_nntp->messages_count] = msg_nntp;
    m_nntp->messages_count++;
  }
  monitor_unlock (mbox->monitor);

  /* Save The message pointer.  */
  message_set_mailbox (msg, mbox, msg_nntp);
  *pmsg = msg_nntp->message = msg;

  return 0;
}

/* There is no explicit call to get the message count.  The count is send on
   a "GROUP" command.  The function is also use as a way to select newsgoupr by other functions.  */
static int
nntp_mailbox_messages_count (mailbox_t mbox, size_t *pcount)
{
  m_nntp_t m_nntp = mbox->data;
  f_nntp_t f_nntp = m_nntp->f_nntp;
  int status = 0;

  status = folder_open (mbox->folder, mbox->flags);
  if (status != 0)
    return status;

  /* Are we already selected ? */
  if (m_nntp == (f_nntp->selected))
    {
      if (pcount)
        *pcount = m_nntp->number;
      return 0;
    }

  /*  Put the mailbox as selected.  */
  f_nntp->selected = m_nntp;

  status = mu_nntp_group (f_nntp->nntp, m_nntp->name, &m_nntp->number, &m_nntp->low, &m_nntp->high, NULL);

  if (pcount)
    *pcount = m_nntp->number;

  return status;
}

/* Update and scanning. FIXME: Is not used */
static int
nntp_is_updated (mailbox_t mbox)
{
  return 1;
}

/* We just simulate by sending a notification for the total msgno.  */
/* FIXME is message is set deleted should we sent a notif ?  */
static int
nntp_mailbox_scan (mailbox_t mbox, size_t msgno, size_t *pcount)
{
  int status;
  size_t i;
  size_t count = 0;

  /* Select first.  */
  status = nntp_mailbox_messages_count (mbox, &count);
  if (pcount)
    *pcount = count;
  if (status != 0)
    return status;
  if (mbox->observable == NULL)
    return 0;
  for (i = msgno; i <= count; i++)
    {
      if (observable_notify (mbox->observable, MU_EVT_MESSAGE_ADD) != 0)
	break;
      if (((i +1) % 10) == 0)
	{
	  observable_notify (mbox->observable, MU_EVT_MAILBOX_PROGRESS);
	}
    }
  return 0;
}

static int
nntp_message_size (message_t msg, size_t *psize)
{
  if (psize)
    *psize = 0;
  return 0;
}

static int
nntp_body_size (body_t body, size_t *psize)
{
  if (psize)
    *psize = 0;

  return 0;
}

/* Not know until the whole message get downloaded.  */
static int
nntp_body_lines (body_t body, size_t *plines)
{
  if (plines)
    *plines = 0;
  return 0;
}

/* Stub to call the fd from body object.  */
static int
nntp_body_get_transport2 (stream_t stream, mu_transport_t *pin, mu_transport_t *pout)
{
  body_t body = stream_get_owner (stream);
  message_t msg = body_get_owner (body);
  msg_nntp_t msg_nntp = message_get_owner (msg);
  return nntp_get_transport2 (msg_nntp, pin, pout);
}

/* Stub to call the fd from message object.  */
static int
nntp_message_get_transport2 (stream_t stream, mu_transport_t *pin, mu_transport_t *pout)
{
  message_t msg = stream_get_owner (stream);
  msg_nntp_t msg_nntp = message_get_owner (msg);
  return nntp_get_transport2 (msg_nntp, pin, pout);
}

static int
nntp_get_transport2 (msg_nntp_t msg_nntp, mu_transport_t *pin, mu_transport_t *pout)
{
  int status = EINVAL;
  if (msg_nntp && msg_nntp->m_nntp
      && msg_nntp->m_nntp->f_nntp && msg_nntp->m_nntp->f_nntp->folder)
    {
      stream_t carrier;
      status = mu_nntp_get_carrier (msg_nntp->m_nntp->f_nntp->nntp, &carrier);
      if (status == 0)
	return stream_get_transport2 (carrier, pin, pout);
    }
  return status;
}

static int
nntp_message_uid (message_t msg,  size_t *puid)
{
  msg_nntp_t msg_nntp = message_get_owner (msg);
  m_nntp_t m_nntp = msg_nntp->m_nntp;
  int status;

  if (puid)
    return 0;

  /* Select first.  */
  status = nntp_mailbox_messages_count (m_nntp->mailbox, NULL);
  if (status != 0)
    return status;

  if (puid)
    *puid = msg_nntp->msgno;
  return 0;
}

static int
nntp_message_uidl (message_t msg, char *buffer, size_t buflen,
		   size_t *pnwriten)
{
  msg_nntp_t msg_nntp = message_get_owner (msg);
  m_nntp_t m_nntp = msg_nntp->m_nntp;
  int status = 0;

  /* Select first.  */
  status = nntp_mailbox_messages_count (m_nntp->mailbox, NULL);
  if (status != 0)
    return status;

  if (msg_nntp->mid)
    {
      size_t len = strlen (msg_nntp->mid);
      if (buffer)
	{
	  buflen--; /* Leave space for the null.  */
	  buflen = (len > buflen) ? buflen : len;
	  memcpy (buffer, msg_nntp->mid, buflen);
	  buffer[buflen] = '\0';
	}
      else
	buflen = len;
    }
  else
    buflen = 0;

  if (pnwriten)
    *pnwriten = buflen;
  return status;
}

/* Message read overload  */
static int
nntp_message_read (stream_t stream, char *buffer, size_t buflen, off_t offset, size_t *plen)
{
  message_t msg = stream_get_owner (stream);
  msg_nntp_t msg_nntp = message_get_owner (msg);
  m_nntp_t m_nntp = msg_nntp->m_nntp;
  f_nntp_t f_nntp = m_nntp->f_nntp;
  int status;
  size_t len = 0;

  /* Start over.  */
  if (plen == NULL)
    plen = &len;

  /* Select first.  */
  status = nntp_mailbox_messages_count (m_nntp->mailbox, NULL);
  if (status != 0)
    return status;

  if (msg_nntp->mstream == NULL)
    {
      status = mu_nntp_article (f_nntp->nntp, msg_nntp->msgno, NULL, &msg_nntp->mid,  &msg_nntp->mstream);
      if (status != 0)
	return status;
    }
  status = stream_read (msg_nntp->mstream, buffer, buflen, offset, plen);
  if (status == 0)
    {
      /* Destroy the stream.  */
      if (*plen == 0)
	{
	  stream_destroy (&msg_nntp->mstream, NULL);
	}
    }
  return status;
}

/* Message read overload  */
static int
nntp_body_read (stream_t stream, char *buffer, size_t buflen, off_t offset, size_t *plen)
{
  body_t body = stream_get_owner (stream);
  message_t msg = body_get_owner (body);
  msg_nntp_t msg_nntp = message_get_owner (msg);
  m_nntp_t m_nntp = msg_nntp->m_nntp;
  f_nntp_t f_nntp = m_nntp->f_nntp;
  int status;
  size_t len = 0;

  /* Start over.  */
  if (plen == NULL)
    plen = &len;

  /* Select first.  */
  status = nntp_mailbox_messages_count (m_nntp->mailbox, NULL);
  if (status != 0)
    return status;

  if (msg_nntp->bstream == NULL)
    {
      status = mu_nntp_body (f_nntp->nntp, msg_nntp->msgno, NULL, &msg_nntp->mid,  &msg_nntp->bstream);
      if (status != 0)
	return status;
    }
  status = stream_read (msg_nntp->bstream, buffer, buflen, offset, plen);
  if (status == 0)
    {
      /* Destroy the stream.  */
      if (*plen == 0)
	{
	  stream_destroy (&msg_nntp->bstream, NULL);
	}
    }
  return status;
}

/* Header read overload  */
static int
nntp_header_fill (header_t header, char *buffer, size_t buflen, off_t offset, size_t *plen)
{
  message_t msg = header_get_owner (header);
  msg_nntp_t msg_nntp = message_get_owner (msg);
  m_nntp_t m_nntp = msg_nntp->m_nntp;
  f_nntp_t f_nntp = m_nntp->f_nntp;
  int status;
  size_t len = 0;

  /* Start over.  */
  if (plen == NULL)
    plen = &len;

  /* Select first.  */
  status = nntp_mailbox_messages_count (m_nntp->mailbox, NULL);
  if (status != 0)
    return status;

  if (msg_nntp->hstream == NULL)
    {
      status = mu_nntp_head (f_nntp->nntp, msg_nntp->msgno, NULL, &msg_nntp->mid,  &msg_nntp->hstream);
      if (status != 0)
	return status;
    }
  status = stream_read (msg_nntp->hstream, buffer, buflen, offset, plen);
  if (status == 0)
    {
      /* Destroy the stream.  */
      if (*plen == 0)
	{
	  stream_destroy (&msg_nntp->hstream, NULL);
	}
    }
  return status;
}

#endif
