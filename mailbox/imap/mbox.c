/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2003 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#ifdef ENABLE_IMAP

#include <errno.h>
#include <string.h>
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#include <stdlib.h>
#include <assert.h>
#include <time.h>

#include <mailutils/address.h>
#include <mailutils/attribute.h>
#include <mailutils/body.h>
#include <mailutils/debug.h>
#include <mailutils/envelope.h>
#include <mailutils/error.h>
#include <mailutils/header.h>
#include <mailutils/message.h>
#include <mailutils/mutil.h>
#include <mailutils/observer.h>
#include <mailutils/property.h>
#include <mailutils/stream.h>

#include <imap0.h>
#include <mailbox0.h>
#include <registrar0.h>
#include <url0.h>

#undef min
#define min(a,b) ((a) < (b) ? (a) : (b))

#define MU_IMAP_CACHE_HEADERS "Bcc Cc Content-Language Content-Transfer-Encoding Content-Type Date From In-Reply-To Message-ID Reference Reply-To Sender Subject To X-UIDL"

/* mailbox_t API.  */
static void mailbox_imap_destroy  __P ((mailbox_t));
static int  mailbox_imap_open     __P ((mailbox_t, int));
static int  mailbox_imap_close    __P ((mailbox_t));
static int  imap_uidvalidity      __P ((mailbox_t, unsigned long *));
static int  imap_uidnext          __P ((mailbox_t, size_t *));
static int  imap_expunge          __P ((mailbox_t));
static int  imap_get_message      __P ((mailbox_t, size_t, message_t *));
static int  imap_messages_count   __P ((mailbox_t, size_t *));
static int  imap_messages_recent  __P ((mailbox_t, size_t *));
static int  imap_message_unseen   __P ((mailbox_t, size_t *));
static int  imap_scan             __P ((mailbox_t, size_t, size_t *));
static int  imap_scan0            __P ((mailbox_t, size_t, size_t *, int));
static int  imap_is_updated       __P ((mailbox_t));
static int  imap_append_message   __P ((mailbox_t, message_t));
static int  imap_append_message0  __P ((mailbox_t, message_t));
static int  imap_copy_message     __P ((mailbox_t, message_t));

/* message_t API.  */
static int  imap_submessage_size  __P ((msg_imap_t, size_t *));
static int  imap_message_size     __P ((message_t, size_t *));
static int  imap_message_lines    __P ((message_t, size_t *));
static int  imap_message_fd       __P ((stream_t, int *, int *));
static int  imap_message_read     __P ((stream_t , char *, size_t, off_t, size_t *));
static int  imap_message_uid      __P ((message_t, size_t *));

/* mime_t API.  */
static int  imap_is_multipart     __P ((message_t, int *));
static int  imap_get_num_parts    __P ((message_t, size_t *));
static int  imap_get_part         __P ((message_t, size_t, message_t *));

/* envelope_t API  */
static int  imap_envelope_sender  __P ((envelope_t, char *, size_t, size_t *));
static int  imap_envelope_date    __P ((envelope_t, char *, size_t, size_t *));

/* attribute_t API  */
static int  imap_attr_get_flags   __P ((attribute_t, int *));
static int  imap_attr_set_flags   __P ((attribute_t, int));
static int  imap_attr_unset_flags __P ((attribute_t, int));

/* header_t API.  */
static int  imap_header_read      __P ((header_t, char*, size_t, off_t, size_t *));
static int  imap_header_get_value __P ((header_t, const char*, char *, size_t, size_t *));
static int  imap_header_get_fvalue __P ((header_t, const char*, char *, size_t, size_t *));

/* body_t API.  */
static int  imap_body_read        __P ((stream_t, char *, size_t, off_t,
					size_t *));
static int  imap_body_size        __P ((body_t, size_t *));
static int  imap_body_lines       __P ((body_t, size_t *));
static int  imap_body_fd          __P ((stream_t, int *, int *));

/* Helpers.  */
static int  imap_get_fd2          __P ((msg_imap_t msg_imap, int *pfd1,
					int *pfd2));
static int  imap_get_message0     __P ((msg_imap_t, message_t *));
static int  fetch_operation       __P ((f_imap_t, msg_imap_t, char *, size_t, size_t *));
static void free_subparts         __P ((msg_imap_t));
static int  flags_to_string       __P ((char **, int));
static int  delete_to_string      __P ((m_imap_t, char **));
static int  is_same_folder        __P ((mailbox_t, message_t));

/* Initialize the concrete object mailbox_t by overloading the function of the
   structure.  */
int
_mailbox_imap_init (mailbox_t mailbox)
{
  m_imap_t m_imap;
  size_t name_len = 0;
  folder_t folder = NULL;

  assert(mailbox);

  folder = mailbox->folder;

  m_imap = mailbox->data = calloc (1, sizeof (*m_imap));
  if (m_imap == NULL)
    return ENOMEM;

  /* Retrieve the name of the mailbox from the URL.  */
  url_get_path (mailbox->url, NULL, 0, &name_len);
  if (name_len == 0)
    {
      /* name "INBOX" is the default.  */
      m_imap->name = calloc (6, sizeof (char));
      strcpy (m_imap->name, "INBOX");
    }
  else
    {
      m_imap->name = calloc (name_len + 1, sizeof (char));
      url_get_path (mailbox->url, m_imap->name, name_len + 1, NULL);
    }

  /* Overload the functions.  */
  mailbox->_destroy = mailbox_imap_destroy;

  mailbox->_open = mailbox_imap_open;
  mailbox->_close = mailbox_imap_close;

  /* Messages.  */
  mailbox->_get_message = imap_get_message;
  mailbox->_append_message = imap_append_message;
  mailbox->_messages_count = imap_messages_count;
  mailbox->_messages_recent = imap_messages_recent;
  mailbox->_message_unseen = imap_message_unseen;
  mailbox->_expunge = imap_expunge;
  mailbox->_uidvalidity = imap_uidvalidity;
  mailbox->_uidnext = imap_uidnext;

  mailbox->_scan = imap_scan;
  mailbox->_is_updated = imap_is_updated;

  /* Get the back pointer of the concrete folder. */
  if (mailbox->folder)
    m_imap->f_imap = mailbox->folder->data;
  /* maibox back pointer.  */
  m_imap->mailbox = mailbox;

  /* Set our properties.  */
  {
    property_t property = NULL;
    mailbox_get_property (mailbox, &property);
    property_set_value (property, "TYPE", "IMAP4", 1);
  }

  return 0;
}

/* Recursive call to free all the subparts of a message.  */
static void
free_subparts (msg_imap_t msg_imap)
{
  size_t i;
  for (i = 0; i < msg_imap->num_parts; i++)
    {
      if (msg_imap->parts[i])
	free_subparts (msg_imap->parts[i]);
    }

  if (msg_imap->message)
    message_destroy (&(msg_imap->message), msg_imap);
  if (msg_imap->parts)
    free (msg_imap->parts);
  if (msg_imap->fheader)
    header_destroy (&msg_imap->fheader, NULL);
  if (msg_imap->internal_date)
    free (msg_imap->internal_date);
  free(msg_imap);
}

/* Give back all the resources. But it does not mean to shutdown the channel
   this is done on the folder.  */
static void
mailbox_imap_destroy (mailbox_t mailbox)
{
  if (mailbox->data)
    {
      m_imap_t m_imap = mailbox->data;
      f_imap_t f_imap = m_imap->f_imap;
      size_t i;

      /* Deselect.  */
      if (m_imap != f_imap->selected)
	f_imap->selected = NULL;

      monitor_wrlock (mailbox->monitor);
      /* Destroy the imap messages and ressources associated to them.  */
      for (i = 0; i < m_imap->imessages_count; i++)
	{
	  if (m_imap->imessages[i])
	    free_subparts (m_imap->imessages[i]);
	}
      if (m_imap->imessages)
	free (m_imap->imessages);
      if (m_imap->name)
	free (m_imap->name);
      free (m_imap);
      mailbox->data = NULL;
      monitor_unlock (mailbox->monitor);
    }
}

/* If the connection was not up it is open by the folder since the stream
   socket is actually created by the folder.  It is not necessary
   to set select the mailbox right away, there are maybe on going operations.
   But on any operation by a particular mailbox, it will be selected first.  */
static int
mailbox_imap_open (mailbox_t mailbox, int flags)
{
  int status = 0;
  m_imap_t m_imap = mailbox->data;
  f_imap_t f_imap = m_imap->f_imap;
  folder_t folder = f_imap->folder;
  struct folder_list folders = { 0, 0 };

  /* m_imap must have been created during mailbox initialization. */
  assert (mailbox->data);
  assert (m_imap->name);

  mailbox->flags = flags;

  if ((status = folder_open (mailbox->folder, flags)))
    return status;

  /* We might not have to SELECT the mailbox, but we need to know it
     exists, and CREATE it if it doesn't, and CREATE is specified in
     the flags.
   */

  switch (m_imap->state)
    {
    case IMAP_NO_STATE:
      m_imap->state = IMAP_LIST;

    case IMAP_LIST:
      status = folder_list (folder, NULL, m_imap->name, &folders);
      if (status != 0)
	{
	  if (status != EAGAIN && status != EINPROGRESS && status != EINTR)
	    m_imap->state = IMAP_NO_STATE;

	  return status;
	}
      m_imap->state = IMAP_NO_STATE;
      if (folders.num)
	return 0;

      if ((flags & MU_STREAM_CREAT) == 0)
	return ENOENT;

      m_imap->state = IMAP_CREATE;

    case IMAP_CREATE:
      switch (f_imap->state)
	{
	case IMAP_NO_STATE:
	  {
	    char *path;
	    size_t len;
	    url_get_path (folder->url, NULL, 0, &len);
	    if (len == 0)
	      return 0;
	    path = calloc (len + 1, sizeof (*path));
	    if (path == NULL)
	      return ENOMEM;
	    url_get_path (folder->url, path, len + 1, NULL);
	    status = imap_writeline (f_imap, "g%u CREATE %s\r\n",
				     f_imap->seq, path);
	    MAILBOX_DEBUG2 (folder, MU_DEBUG_PROT, "g%u CREATE %s\n",
			    f_imap->seq, path);
	    f_imap->seq++;
	    free (path);
	    if (status != 0)
	      {
		m_imap->state = f_imap->state = IMAP_NO_STATE;
		return status;
	      }
	    f_imap->state = IMAP_CREATE;
	  }

	case IMAP_CREATE:
	  status = imap_send (f_imap);
	  if (status != 0)
	    {
	      if (status != EAGAIN && status != EINPROGRESS
		  && status != EINTR)
		m_imap->state = f_imap->state = IMAP_NO_STATE;

	      return status;
	    }
	  f_imap->state = IMAP_CREATE_ACK;

	case IMAP_CREATE_ACK:
	  status = imap_parse (f_imap);
	  if (status != 0)
	    {
	      if (status == EINVAL)
		status = EACCES;

	      if (status != EAGAIN && status != EINPROGRESS
		  && status != EINTR)
		m_imap->state = f_imap->state = IMAP_NO_STATE;

	      return status;
	    }
	  f_imap->state = IMAP_NO_STATE;
	  break;

	default:
	  status = EINVAL;
	  break;
	}
      m_imap->state = IMAP_NO_STATE;
      break;

    default:
      status = EINVAL;
      break;
    }

  return status;
}

/* We can not close the folder in term of shuting down the connection but if
   we were the selected mailbox we send the close and deselect ourself.
   The CLOSE is also use to expunge instead of sending expunge.  */
static int
mailbox_imap_close (mailbox_t mailbox)
{
  m_imap_t m_imap = mailbox->data;
  f_imap_t f_imap = m_imap->f_imap;
  int status = 0;

  /* If we are not the selected mailbox, just close the stream.  */
  if (m_imap != f_imap->selected)
    return folder_close (mailbox->folder);

  /* Select first.  */
  status = imap_messages_count (mailbox, NULL);
  if (status != 0)
    return status;

  switch (f_imap->state)
    {
    case IMAP_NO_STATE:
      status = imap_writeline (f_imap, "g%d CLOSE\r\n", f_imap->seq++);
      CHECK_ERROR (f_imap, status);
      MAILBOX_DEBUG0 (mailbox, MU_DEBUG_PROT, f_imap->buffer);
      f_imap->state = IMAP_CLOSE;

    case IMAP_CLOSE:
      status = imap_send (f_imap);
      CHECK_EAGAIN (f_imap, status);
      f_imap->state = IMAP_CLOSE_ACK;

    case IMAP_CLOSE_ACK:
      {
	size_t i;
	status = imap_parse (f_imap);
	CHECK_EAGAIN (f_imap, status);
	MAILBOX_DEBUG0 (mailbox, MU_DEBUG_PROT, f_imap->buffer);

	monitor_wrlock (mailbox->monitor);
	/* Destroy the imap messages and ressources associated to them.  */
	for (i = 0; i < m_imap->imessages_count; i++)
	  {
	    if (m_imap->imessages[i])
	      free_subparts (m_imap->imessages[i]);
	  }
	if (m_imap->imessages)
	  free (m_imap->imessages);
	m_imap->imessages = NULL;
	m_imap->imessages_count = 0;
	m_imap->messages_count = 0;
	m_imap->recent = 0;
	m_imap->unseen = 0;
	/* Clear the callback string structure.  */
	stream_truncate (f_imap->string.stream, 0);
	f_imap->string.offset = 0;
	f_imap->string.nleft = 0;
	f_imap->string.type = IMAP_NO_STATE;
	f_imap->string.msg_imap = NULL;
	monitor_unlock (mailbox->monitor);
      }
      break;

    default:
      /* mu_error ("imap_close unknown state: reconnect\n");*/
      break;
    }

  /* Deselect.  */
  f_imap->selected = NULL;

  f_imap->state = IMAP_NO_STATE;
  return folder_close (mailbox->folder);
}

/* Construction of the message_t, nothing else is done then this setup.  To
   clarify this is different from say message_get_part().  This call is for the
   mailbox and we are setting up the message_t structure.  */
static int
imap_get_message (mailbox_t mailbox, size_t msgno, message_t *pmsg)
{
  m_imap_t m_imap = mailbox->data;
  msg_imap_t msg_imap;
  int status = 0;

  if (pmsg == NULL || msgno == 0 || msgno > m_imap->messages_count)
    return EINVAL;

  /* Check to see if we have already this message.  */
  monitor_rdlock (mailbox->monitor);
  {
    size_t i;
    for (i = 0; i < m_imap->imessages_count; i++)
      {
	if (m_imap->imessages[i])
	  {
	    if (m_imap->imessages[i]->num == msgno)
	      {
		*pmsg = m_imap->imessages[i]->message;
		monitor_unlock (mailbox->monitor);
		return 0;
	      }
	  }
      }
  }
  monitor_unlock (mailbox->monitor);

  /* Allocate a concrete imap message.  */
  msg_imap = calloc (1, sizeof *msg_imap);
  if (msg_imap == NULL)
    return ENOMEM;
  /* Back pointer.  */
  msg_imap->m_imap = m_imap;
  msg_imap->num = msgno;
  status = imap_get_message0 (msg_imap, pmsg);
  if (status == 0)
    {
      /* Add it to the list.  */
      monitor_wrlock (mailbox->monitor);
      {
	msg_imap_t *m ;
	m = realloc (m_imap->imessages,
		     (m_imap->imessages_count + 1) * sizeof *m);
	if (m == NULL)
	  {
	    message_destroy (pmsg, msg_imap);
	    monitor_unlock (mailbox->monitor);
	    return ENOMEM;
	  }
	m_imap->imessages = m;
	m_imap->imessages[m_imap->imessages_count] = msg_imap;
	m_imap->imessages_count++;
      }
      monitor_unlock (mailbox->monitor);

      msg_imap->message = *pmsg;
    }
  else
    free (msg_imap);
  return status;
}

/* Set all the message_t functions and parts.  */
static int
imap_get_message0 (msg_imap_t msg_imap, message_t *pmsg)
{
  int status = 0;
  message_t msg = NULL;
  mailbox_t mailbox = msg_imap->m_imap->mailbox;

  /* Create the message and its stream.  */
  {
    stream_t stream = NULL;
    if ((status = message_create (&msg, msg_imap)) != 0
        || (status = stream_create (&stream, mailbox->flags, msg)) != 0)
      {
        stream_destroy (&stream, msg);
        message_destroy (&msg, msg_imap);
        return status;
      }
    stream_setbufsiz (stream, 128);
    stream_set_read (stream, imap_message_read, msg);
    stream_set_fd (stream, imap_message_fd, msg);
    message_set_stream (msg, stream, msg_imap);
    message_set_size (msg, imap_message_size, msg_imap);
    message_set_lines (msg, imap_message_lines, msg_imap);
  }

  /* Create the header.  */
  {
    header_t header = NULL;
    if ((status = header_create (&header, NULL, 0,  msg)) != 0)
      {
        message_destroy (&msg, msg_imap);
        return status;
      }
    header_set_fill (header, imap_header_read, msg);
    header_set_get_value (header, imap_header_get_value, msg);
    header_set_get_fvalue (header, imap_header_get_fvalue, msg);
    message_set_header (msg, header, msg_imap);
  }

  /* Create the attribute.  */
  {
    attribute_t attribute;
    status = attribute_create (&attribute, msg);
    if (status != 0)
      {
        message_destroy (&msg, msg_imap);
        return status;
      }
    attribute_set_get_flags (attribute, imap_attr_get_flags, msg);
    attribute_set_set_flags (attribute, imap_attr_set_flags, msg);
    attribute_set_unset_flags (attribute, imap_attr_unset_flags, msg);
    message_set_attribute (msg, attribute, msg_imap);
  }

  /* Create the body and its stream.  */
  {
    body_t body = NULL;
    stream_t stream = NULL;
    if ((status = body_create (&body, msg)) != 0
        || (status = stream_create (&stream, mailbox->flags, body)) != 0)
      {
        body_destroy (&body, msg);
        stream_destroy (&stream, body);
        message_destroy (&msg, msg_imap);
        return status;
      }
    stream_setbufsiz (stream, 128);
    stream_set_read (stream, imap_body_read, body);
    stream_set_fd (stream, imap_body_fd, body);
    body_set_size (body, imap_body_size, msg);
    body_set_lines (body, imap_body_lines, msg);
    body_set_stream (body, stream, msg);
    message_set_body (msg, body, msg_imap);
  }

  /* Set the envelope.  */
  {
    envelope_t envelope= NULL;
    status = envelope_create (&envelope, msg);
    if (status != 0)
      {
        message_destroy (&msg, msg_imap);
        return status;
      }
    envelope_set_sender (envelope, imap_envelope_sender, msg);
    envelope_set_date (envelope, imap_envelope_date, msg);
    message_set_envelope (msg, envelope, msg_imap);
  }

  /* Set the mime handling.  */
  message_set_is_multipart (msg, imap_is_multipart, msg_imap);
  message_set_get_num_parts (msg, imap_get_num_parts, msg_imap);
  message_set_get_part (msg, imap_get_part, msg_imap);

  /* Set the UID on the message. */
  message_set_uid (msg, imap_message_uid, msg_imap);
  message_set_mailbox (msg, mailbox, msg_imap);

  /* We are done here.  */
  *pmsg = msg;
  return 0;
}

static int
imap_message_unseen (mailbox_t mailbox, size_t *punseen)
{
  m_imap_t m_imap = mailbox->data;
  *punseen = m_imap->unseen;
  return 0;
}

static int
imap_messages_recent (mailbox_t mailbox, size_t *precent)
{
  m_imap_t m_imap = mailbox->data;
  *precent = m_imap->recent;
  return 0;
}

static int
imap_uidvalidity (mailbox_t mailbox, unsigned long *puidvalidity)
{
  m_imap_t m_imap = mailbox->data;
  *puidvalidity = m_imap->uidvalidity;
  return 0;
}

static int
imap_uidnext (mailbox_t mailbox, size_t *puidnext)
{
  m_imap_t m_imap = mailbox->data;
  *puidnext = m_imap->uidnext;
  return 0;
}

/* There is no explicit call to get the message count.  The count is send on
   a SELECT/EXAMINE command it is also sent async, meaning it will be piggy
   back on other server response as an untag "EXIST" response.  The
   function is also use as a way to select mailbox by other functions.  */
static int
imap_messages_count (mailbox_t mailbox, size_t *pnum)
{
  m_imap_t m_imap = mailbox->data;
  f_imap_t f_imap = m_imap->f_imap;
  int status = 0;

  /* FIXME: It is debatable if we should reconnect when the connection
     timeout or die.  Probably for timeout client should ping i.e. send
     a NOOP via imap_is_updated() function to keep the connection alive.  */
  status = folder_open (mailbox->folder, mailbox->flags);
  if (status != 0)
    return status;

  /* Are we already selected ? */
  if (m_imap == (f_imap->selected))
    {
      if (pnum)
	*pnum = m_imap->messages_count;
      return 0;
    }

  /*  Put the mailbox as selected.  */
  f_imap->selected = m_imap;

  switch (f_imap->state)
    {
    case IMAP_NO_STATE:
      status = imap_writeline (f_imap, "g%d SELECT %s\r\n",
			       f_imap->seq++, m_imap->name);
      CHECK_ERROR (f_imap, status);
      MAILBOX_DEBUG0 (mailbox, MU_DEBUG_PROT, f_imap->buffer);
      f_imap->state = IMAP_SELECT;

    case IMAP_SELECT:
      status = imap_send (f_imap);
      CHECK_EAGAIN (f_imap, status);
      f_imap->state = IMAP_SELECT_ACK;

    case IMAP_SELECT_ACK:
      status = imap_parse (f_imap);
      CHECK_EAGAIN (f_imap, status);
      MAILBOX_DEBUG0 (mailbox, MU_DEBUG_PROT, f_imap->buffer);
      break;

    default:
      /*mu_error ("imap_message_count unknown state: reconnect\n");*/
      break;
    }

  if (pnum)
    *pnum = m_imap->messages_count;

  f_imap->state = IMAP_NO_STATE;
  return status;
}

static int
imap_scan (mailbox_t mailbox, size_t msgno, size_t *pcount)
{
  return imap_scan0 (mailbox, msgno, pcount , 1);
}

/* Usually when this function is call it is because there is an oberver
   attach an the client is try to build some sort of list/tree header
   as the scanning progress.  But doing this for each message can be
   time consuming and inefficient.  So we bundle all the request
   into one and ask the server for everything "FETCH 1:*".  The good
   side is that everything will be faster and we do not do lot of small
   transcation but rather a big one.  The bad thing is that every thing
   will be cache in the structure using a lot of memory.  */
static int
imap_scan0 (mailbox_t mailbox, size_t msgno, size_t *pcount, int notif)
{
  int status;
  size_t i;
  size_t count = 0;
  m_imap_t m_imap = mailbox->data;
  f_imap_t f_imap = m_imap->f_imap;

  /* Selected.  */
  status = imap_messages_count (mailbox, &count);
  if (pcount)
    *pcount = count;
  if (status != 0)
    return status;

  /* No need to scan, there is no messages. */
  if (count == 0)
    return 0;

  switch (f_imap->state)
    {
    case IMAP_NO_STATE:
      status = imap_writeline (f_imap,
			       "g%d FETCH 1:* (FLAGS RFC822.SIZE BODY.PEEK[HEADER.FIELDS (%s)])\r\n",
			       f_imap->seq++, MU_IMAP_CACHE_HEADERS);
      CHECK_ERROR (f_imap, status);
      MAILBOX_DEBUG0 (mailbox, MU_DEBUG_PROT, f_imap->buffer);
      f_imap->state = IMAP_SCAN;

    case IMAP_SCAN:
      status = imap_send (f_imap);
      CHECK_EAGAIN (f_imap, status);
      f_imap->state = IMAP_SCAN_ACK;
      /* Clear the callback string structure.  */
      stream_truncate (f_imap->string.stream, 0);
      f_imap->string.offset = 0;
      f_imap->string.nleft = 0;
      f_imap->string.type = IMAP_NO_STATE;
      f_imap->string.msg_imap = NULL;

    case IMAP_SCAN_ACK:
      status = imap_parse (f_imap);
      CHECK_EAGAIN (f_imap, status);
      MAILBOX_DEBUG0 (mailbox, MU_DEBUG_PROT, f_imap->buffer);
      /* Clear the callback string structure.  */
      stream_truncate (f_imap->string.stream, 0);
      f_imap->string.offset = 0;
      f_imap->string.nleft = 0;
      f_imap->string.type = IMAP_NO_STATE;
      f_imap->string.msg_imap = NULL;
      break;

    default:
      /*mu_error ("imap_scan unknown state: reconnect\n");*/
      return EINVAL;
    }

  f_imap->state = IMAP_NO_STATE;

  /* Do not send notifications.  */
  if (!notif)
    return 0;

  /* If no callbacks bail out early.  */
  if (mailbox->observable == NULL)
    return 0;

  for (i = msgno; i <= count; i++)
    {
      if (observable_notify (mailbox->observable, MU_EVT_MESSAGE_ADD) != 0)
	break;
      if (((i + 1) % 100) == 0)
	{
	  observable_notify (mailbox->observable, MU_EVT_MAILBOX_PROGRESS);
	}
    }
  return 0;
}

/* Send a NOOP and see if the count has changed.  */
static int
imap_is_updated (mailbox_t mailbox)
{
  m_imap_t m_imap = mailbox->data;
  size_t oldcount = m_imap->messages_count;
  f_imap_t f_imap = m_imap->f_imap;
  int status = 0;

  /* Selected.  */
  status = imap_messages_count (mailbox, &oldcount);
  if (status != 0)
    return status;

  /* Send a noop, and let imap piggy pack the information.  */
  switch (f_imap->state)
    {
    case IMAP_NO_STATE:
      status = imap_writeline (f_imap, "g%d NOOP\r\n", f_imap->seq++);
      CHECK_ERROR (f_imap, status);
      MAILBOX_DEBUG0 (mailbox, MU_DEBUG_PROT, f_imap->buffer);
      f_imap->state = IMAP_NOOP;

    case IMAP_NOOP:
      status = imap_send (f_imap);
      CHECK_EAGAIN (f_imap, status);
      f_imap->state = IMAP_NOOP_ACK;

    case IMAP_NOOP_ACK:
      status = imap_parse (f_imap);
      CHECK_EAGAIN (f_imap, status);
      MAILBOX_DEBUG0 (mailbox, MU_DEBUG_PROT, f_imap->buffer);
      break;

    default:
      /*mu_error ("imap_noop unknown state: reconnect\n"); */
      break;
    }
  f_imap->state = IMAP_NO_STATE;
  return (oldcount == m_imap->messages_count);
}


static int
imap_expunge (mailbox_t mailbox)
{
  int status;
  m_imap_t m_imap = mailbox->data;
  f_imap_t f_imap = m_imap->f_imap;

  /* Select first.  */
  status = imap_messages_count (mailbox, NULL);
  if (status != 0)
    return status;

  switch (f_imap->state)
    {
    case IMAP_NO_STATE:
      {
	char *set = NULL;
	status = delete_to_string (m_imap, &set);
	CHECK_ERROR (f_imap, status);
	if (set == NULL || *set == '\0')
	  {
	    if (set)
	      free (set);
	    return 0;
	  }
	status = imap_writeline (f_imap,
				 "g%d STORE %s +FLAGS.SILENT (\\Deleted)\r\n",
				 f_imap->seq++, set);
	free (set);
	CHECK_ERROR (f_imap, status);
	MAILBOX_DEBUG0 (m_imap->mailbox, MU_DEBUG_PROT, f_imap->buffer);
	f_imap->state = IMAP_STORE;
      }

      /* Send DELETE.  */
    case IMAP_STORE:
      status = imap_send (f_imap);
      CHECK_EAGAIN (f_imap, status);
      f_imap->state = IMAP_STORE_ACK;

    case IMAP_STORE_ACK:
      status = imap_parse (f_imap);
      CHECK_EAGAIN (f_imap, status);
      MAILBOX_DEBUG0 (m_imap->mailbox, MU_DEBUG_PROT, f_imap->buffer);
      f_imap->state = IMAP_NO_STATE;

    case IMAP_EXPUNGE:
    case IMAP_EXPUNGE_ACK:
      status = imap_writeline (f_imap, "g%d EXPUNGE\r\n", f_imap->seq++);
      CHECK_ERROR (f_imap, status);
      status = imap_send (f_imap);
      CHECK_EAGAIN (f_imap, status);

      /* Rescan after expunging but do not trigger the observers.  */
    case IMAP_SCAN:
    case IMAP_SCAN_ACK:
      status = imap_scan0 (mailbox, 1, NULL, 0);
      CHECK_EAGAIN (f_imap, status);

    default:
      /* mu_error ("imap_expunge: unknown state\n"); */
      break;
    }

  return status;
}

/* FIXME: Not ___Nonblocking___ safe.  */
/* DANGER:  The message_t object makes no guaranty about the size and the lines
   that it returns, if its pointing to non-local file messages, so we
   make a local copy.  */
static int
imap_append_message (mailbox_t mailbox, message_t msg)
{
  int status = 0;
  m_imap_t m_imap = mailbox->data;
  f_imap_t f_imap = m_imap->f_imap;

  /* FIXME: It is debatable if we should reconnect when the connection
   timeout or die.  For timeout client should ping i.e. send
   a NOOP via imap_is_updated() function to keep the connection alive.  */
  status = folder_open (mailbox->folder, mailbox->flags);
  if (status != 0)
    return status;

  /* FIXME: Can we append to self.  */

  /* Check to see if we are selected. If the message was not modified
     and came from the same imap folder. use COPY.*/
  if (f_imap->selected != m_imap && !message_is_modified (msg)
      && is_same_folder (mailbox, msg))
    return imap_copy_message (mailbox, msg);

  /* copy the message to local disk by createing a floating message.  */
  {
    message_t message = NULL;

    status = message_create_copy(&message, msg);

    if (status == 0)
      status = imap_append_message0 (mailbox, message);
    message_destroy (&message, NULL);
  }
  return status;
}

/* Ok this mean that the message is coming from somewhere else.  IMAP
   is very susceptible on the size, example:
   A003 APPEND saved-messages (\Seen) {310}
   if the server does not get the right size advertise in the string literal
   it will misbehave.  Sine we are assuming that the message will be
   in native file system format meaning ending with NEWLINE, we will have
   to do the calculation.  But what is worse; the value return
   by message_size () and message_lines () are no mean exact but rather
   a gross approximation for certain type of mailbox.  So the sane
   thing to do is to save the message in temporary file, this we say
   we guarantee the size of the message.  */
static int
imap_append_message0 (mailbox_t mailbox, message_t msg)
{
  size_t total;
  int status = 0;
  m_imap_t m_imap = mailbox->data;
  f_imap_t f_imap = m_imap->f_imap;

  switch (f_imap->state)
    {
    case IMAP_NO_STATE:
      {
	size_t lines, size;
	char *path;
	char *abuf = malloc (1);
	/* Get the desired flags attribute.  */
	if (abuf == NULL)
	  return ENOMEM;
	*abuf = '\0';
	{
	  attribute_t attribute = NULL;
	  int flags = 0;
	  message_get_attribute (msg, &attribute);
	  attribute_get_flags (attribute, &flags);
	  status = flags_to_string (&abuf, flags);
	  if (status != 0)
	    return status;
	  /* Put the surrounding parenthesis, wu-IMAP is sensible to this.  */
	  {
	    char *tmp = calloc (strlen (abuf) + 3, 1);
	    if (tmp == NULL)
	      {
		free (abuf);
		return ENOMEM;
	      }
	    sprintf (tmp, "(%s)", abuf);
	    free (abuf);
	    abuf = tmp;
	  }
	}

	/* Get the mailbox filepath.  */
	{
	  size_t n = 0;
	  url_get_path (mailbox->url, NULL, 0, &n);
	  if (n == 0)
	    {
	      if (!(path = strdup ("INBOX")))
		{
		  free (abuf);
		  return ENOMEM;
		}
	    }
	  else
	    {
	      path = calloc (n + 1, sizeof (*path));
	      if (path == NULL)
		{
		  free (abuf);
		  return ENOMEM;
		}
	      url_get_path (mailbox->url, path, n + 1, NULL);
	    }
	}

	/* FIXME: we need to get the envelope_date and use it.
	   currently it is ignored.  */

	/* Get the total size, assuming that it is in UNIX format.  */
	lines = size = 0;
	message_size (msg, &size);
	message_lines (msg, &lines);
	total = size + lines;
	status = imap_writeline (f_imap, "g%d APPEND %s %s {%d}\r\n",
				 f_imap->seq++, path, abuf, size + lines);
	free (abuf);
	free (path);
	CHECK_ERROR (f_imap, status);
	MAILBOX_DEBUG0 (mailbox, MU_DEBUG_PROT, f_imap->buffer);
	f_imap->state = IMAP_APPEND;
      }

    case IMAP_APPEND:
      status = imap_send (f_imap);
      CHECK_EAGAIN (f_imap, status);
      f_imap->state = IMAP_APPEND_CONT;

    case IMAP_APPEND_CONT:
      status = imap_parse (f_imap);
      CHECK_EAGAIN (f_imap, status);
      MAILBOX_DEBUG0 (mailbox, MU_DEBUG_PROT, f_imap->buffer);
      /* If we did not receive the continuation token, it is an error
         bail out.  */
      if (f_imap->buffer[0] != '+')
	{
	  status = EACCES;
	  break;
	}
      f_imap->state = IMAP_APPEND_SEND;

    case IMAP_APPEND_SEND:
      {
	stream_t stream = NULL;
	off_t off = 0;
	size_t n = 0;
	char buffer[255];
	message_get_stream (msg, &stream);
	while (stream_readline (stream, buffer, sizeof buffer, off, &n) == 0
	       && n > 0)
	  {
	    if (buffer[n - 1] == '\n')
	      {
		buffer[n - 1] = '\0';
		status = imap_writeline (f_imap, "%s\r\n", buffer);
	      }
	    else
	      imap_writeline (f_imap, "%s", buffer);
	    off += n;
	    status = imap_send (f_imap);
	    CHECK_EAGAIN (f_imap, status);
	  }
	f_imap->state = IMAP_APPEND_ACK;
      }
      /* !@#%$ UW-IMAP server hack: insists on the last line.  */
      imap_writeline (f_imap, "\n");
      status = imap_send (f_imap);
      CHECK_EAGAIN (f_imap, status);

    case IMAP_APPEND_ACK:
      status = imap_parse (f_imap);
      CHECK_EAGAIN (f_imap, status);
      MAILBOX_DEBUG0 (m_imap->mailbox, MU_DEBUG_PROT, f_imap->buffer);

    default:
      /* mu_error ("imap_append: unknown state\n"); */
      break;
    }
  f_imap->state = IMAP_NO_STATE;
  return status;
}

/* If the message is on the same server.  Use the COPY command much more
   efficient.  */
static int
imap_copy_message (mailbox_t mailbox, message_t msg)
{
  m_imap_t m_imap = mailbox->data;
  f_imap_t f_imap = m_imap->f_imap;
  msg_imap_t msg_imap = message_get_owner (msg);
  int status = 0;

  /* FIXME: It is debatable if we should reconnect when the connection
   timeout or die.  For timeout client should ping i.e. send
   a NOOP via imap_is_updated() function to keep the connection alive.  */
  status = folder_open (mailbox->folder, mailbox->flags);
  if (status != 0)
    return status;

  switch (f_imap->state)
    {
    case IMAP_NO_STATE:
      {
	char *path;
	size_t n = 0;
	/* Check for a valid mailbox name.  */
	url_get_path (mailbox->url, NULL, 0, &n);
	if (n == 0)
	  return EINVAL;
	path = calloc (n + 1, sizeof (*path));
	if (path == NULL)
	  return ENOMEM;
	url_get_path (mailbox->url, path, n + 1, NULL);
	status = imap_writeline (f_imap, "g%d COPY %d %s\r\n", f_imap->seq++,
				 msg_imap->num, path);
	free (path);
	CHECK_ERROR (f_imap, status);
	MAILBOX_DEBUG0 (mailbox, MU_DEBUG_PROT, f_imap->buffer);
	f_imap->state = IMAP_COPY;
      }

    case IMAP_COPY:
      status = imap_send (f_imap);
      CHECK_EAGAIN (f_imap, status);
      f_imap->state = IMAP_COPY_ACK;

    case IMAP_COPY_ACK:
      status = imap_parse (f_imap);
      CHECK_EAGAIN (f_imap, status);
      MAILBOX_DEBUG0 (mailbox, MU_DEBUG_PROT, f_imap->buffer);

    default:
      break;
    }
  f_imap->state = IMAP_NO_STATE;
  return status;
}

/* Message read overload  */
static int
imap_message_read (stream_t stream, char *buffer, size_t buflen,
		   off_t offset, size_t *plen)
{
  message_t msg = stream_get_owner (stream);
  msg_imap_t msg_imap = message_get_owner (msg);
  m_imap_t m_imap = msg_imap->m_imap;
  f_imap_t f_imap = m_imap->f_imap;
  char *oldbuf = NULL;
  char newbuf[2];
  int status;

  /* This is so annoying, a buffer len of 1 is a killer. If you have for
     example "\n" to retrieve from the server, IMAP will transform this to
     "\r\n" and since you ask for only 1, the server will send '\r' only.
     And ... '\r' will be stripped by (imap_readline()) the number of char
     read will be 0 which means we're done .... sigh ...  So we guard by at
     least ask for 2 chars.  */
  if (buflen == 1)
    {
      oldbuf = buffer;
      buffer = newbuf;
      buflen = 2;
    }

  /* Start over.  */
  if (offset == 0)
    msg_imap->message_lines = 0;

  status = imap_messages_count (m_imap->mailbox, NULL);
  if (status != 0)
    return status;

  /* Select first.  */
  if (f_imap->state == IMAP_NO_STATE)
    {
      char *section = NULL;

      if (msg_imap->part)
	section = section_name (msg_imap);

      /* We have strip the \r, but the offset on the imap server is with that
	 octet(CFLF) so add it in the offset, it's the number of lines.  */
      status = imap_writeline (f_imap,
			       "g%d FETCH %d BODY.PEEK[%s]<%d.%d>\r\n",
			       f_imap->seq++, msg_imap->num,
			       (section) ? section : "",
			       offset + msg_imap->message_lines, buflen);
      if (section)
	free (section);
      CHECK_ERROR (f_imap, status);
      MAILBOX_DEBUG0 (m_imap->mailbox, MU_DEBUG_PROT, f_imap->buffer);
      f_imap->state = IMAP_FETCH;
    }
  status = fetch_operation (f_imap, msg_imap, buffer, buflen, plen);

  if (oldbuf)
    oldbuf[0] = buffer[0];
  return status;
}

static int
imap_message_lines (message_t msg, size_t *plines)
{
  msg_imap_t msg_imap = message_get_owner (msg);
  if (plines && msg_imap)
    {
      if (msg_imap->message_lines == 0)
	*plines = msg_imap->body_lines + msg_imap->header_lines;
      else
	*plines = msg_imap->message_lines;
    }
  return 0;
}

/* Sometimes a message is just a place container for other sub parts.
   In those cases imap bodystructure does not set the message_size aka
   the body_size.  But we can calculate it since the message_size
   is the sum of its subparts.  */
static int
imap_submessage_size (msg_imap_t msg_imap, size_t *psize)
{
  if (psize)
    {
      *psize = 0;
      if (msg_imap->message_size == 0)
	{
	  size_t i, size;
	  for (size = i = 0; i < msg_imap->num_parts; i++, size = 0)
	    {
	      if (msg_imap->parts[i])
		imap_submessage_size (msg_imap->parts[i], &size);
	      *psize += size;
	    }
	}
      else
	*psize = (msg_imap->message_size + msg_imap->header_size)
	  - msg_imap->message_lines;
    }
  return 0;
}

static int
imap_message_size (message_t msg, size_t *psize)
{
  msg_imap_t msg_imap = message_get_owner (msg);
  m_imap_t m_imap = msg_imap->m_imap;
  f_imap_t f_imap = m_imap->f_imap;
  int status = 0;;

  status = imap_messages_count (m_imap->mailbox, NULL);
  if (status != 0)
    return status;

  /* If there is a parent it means it is a sub message, IMAP does not give
     the full size of mime messages, so the message_size retrieved from
     doing a bodystructure represents rather the body_size.  */
  if (msg_imap->parent)
    return imap_submessage_size (msg_imap, psize);

  if (msg_imap->message_size == 0)
    {
      /* Select first.  */
      if (f_imap->state == IMAP_NO_STATE)
	{
	  /* We strip the \r, but the offset/size on the imap server is with
	     that octet so add it in the offset, since it's the number of
	     lines.  */
	  status = imap_writeline (f_imap,
				   "g%d FETCH %d RFC822.SIZE\r\n",
				   f_imap->seq++, msg_imap->num);
	  CHECK_ERROR (f_imap, status);
	  MAILBOX_DEBUG0 (m_imap->mailbox, MU_DEBUG_PROT, f_imap->buffer);
	  f_imap->state = IMAP_FETCH;
	}
      status = fetch_operation (f_imap, msg_imap, 0, 0, 0);
    }

  if (status == 0)
    {
      if (psize)
	*psize = msg_imap->message_size - msg_imap->message_lines;
    }
  return status;
}

static int
imap_message_uid (message_t msg, size_t *puid)
{
  msg_imap_t msg_imap = message_get_owner (msg);
  m_imap_t m_imap = msg_imap->m_imap;
  f_imap_t f_imap = m_imap->f_imap;
  int status;

  if (puid)
    return 0;

  /* Select first.  */
  status = imap_messages_count (m_imap->mailbox, NULL);
  if (status != 0)
    return status;

  if (f_imap->state == IMAP_NO_STATE)
    {
      if (msg_imap->uid)
	{
	  *puid = msg_imap->uid;
	  return 0;
	}
      status = imap_writeline (f_imap, "g%d FETCH %d UID\r\n",
			       f_imap->seq++, msg_imap->num);
      CHECK_ERROR (f_imap, status);
      MAILBOX_DEBUG0 (m_imap->mailbox, MU_DEBUG_PROT, f_imap->buffer);
      f_imap->state = IMAP_FETCH;
    }
  status = fetch_operation (f_imap, msg_imap, 0, 0, 0);
  if (status != 0)
    return status;
  *puid = msg_imap->uid;
  return 0;
}

static int
imap_message_fd (stream_t stream, int *pfd, int *pfd2)
{
  message_t msg = stream_get_owner (stream);
  msg_imap_t msg_imap = message_get_owner (msg);
  return imap_get_fd2 (msg_imap, pfd, pfd2);
}

/* Mime.  */
static int
imap_is_multipart (message_t msg, int *ismulti)
{
  msg_imap_t msg_imap = message_get_owner (msg);
  m_imap_t m_imap = msg_imap->m_imap;
  f_imap_t f_imap = m_imap->f_imap;
  int status;

  /* Select first.  */
  status = imap_messages_count (m_imap->mailbox, NULL);
  if (status != 0)
    return status;

  if (f_imap->state == IMAP_NO_STATE)
    {
      if (msg_imap->num_parts || msg_imap->part)
	{
	  if (ismulti)
	    *ismulti = (msg_imap->num_parts > 1);
	  return 0;
	}
      status = imap_writeline (f_imap,
			       "g%d FETCH %d BODYSTRUCTURE\r\n",
			       f_imap->seq++, msg_imap->num);
      CHECK_ERROR (f_imap, status);
      MAILBOX_DEBUG0 (m_imap->mailbox, MU_DEBUG_PROT, f_imap->buffer);
      f_imap->state = IMAP_FETCH;
    }
  status = fetch_operation (f_imap, msg_imap, 0, 0, 0);
  if (status != 0)
    return status;
  if (ismulti)
    *ismulti = (msg_imap->num_parts > 1);
  return 0;
}

static int
imap_get_num_parts (message_t msg, size_t *nparts)
{
  msg_imap_t msg_imap = message_get_owner (msg);
  if (msg_imap)
    {
      if (msg_imap->num_parts == 0)
	{
	  int status = imap_is_multipart (msg, NULL);
	  if (status != 0)
	    return status;
	}
      if (nparts)
	*nparts = (msg_imap->num_parts == 0) ? 1 : msg_imap->num_parts;
    }
  return 0;
}

static int
imap_get_part (message_t msg, size_t partno, message_t *pmsg)
{
  msg_imap_t msg_imap = message_get_owner (msg);
  int status = 0;

  if (msg_imap->num_parts == 0)
    {
      status = imap_get_num_parts (msg, NULL);
      if (status != 0)
	return status;
    }

  if (partno <= msg_imap->num_parts)
    {
      if (msg_imap->parts[partno - 1]->message)
	{
	  if (pmsg)
	    *pmsg = msg_imap->parts[partno - 1]->message;
	}
      else
	{
	  message_t message;
	  status = imap_get_message0 (msg_imap->parts[partno - 1], &message);
	  if (status == 0)
	    {
	      header_t header;
	      message_get_header (message, &header);
	      header_set_get_value (header, NULL, message);
	      message_set_stream (message, NULL, msg_imap->parts[partno - 1]);
	      /* message_set_size (message, NULL, msg_imap->parts[partno - 1]); */
	      msg_imap->parts[partno - 1]->message = message;
	      if (pmsg)
		*pmsg = message;
	    }
	}
    }
  else
    {
      if (pmsg)
	*pmsg = msg_imap->message;
    }
  return status;
}

/* Envelope.  */
static int
imap_envelope_sender (envelope_t envelope, char *buffer, size_t buflen,
		      size_t *plen)
{
  message_t msg = envelope_get_owner (envelope);
  header_t header;
  int status;

  if (buflen == 0)
    return 0;

  message_get_header (msg, &header);
  status = header_get_value (header, MU_HEADER_SENDER, buffer, buflen, plen);
  if (status == EAGAIN)
    return status;
  else if (status != 0)
    status = header_get_value (header, MU_HEADER_FROM, buffer, buflen, plen);
  if (status == 0)
    {
      address_t address;
      if (address_create (&address, buffer) == 0)
	{
	  address_get_email (address, 1, buffer, buflen, plen);
	  address_destroy (&address);
	}
    }
  else if (status != EAGAIN)
    {
      strncpy (buffer, "Unknown", buflen)[buflen - 1] = '0';
      if (plen)
	*plen = strlen (buffer);
    }
  return status;
}

static int
imap_envelope_date (envelope_t envelope, char *buffer, size_t buflen,
		    size_t *plen)
{
  message_t msg = envelope_get_owner (envelope);
  msg_imap_t msg_imap = message_get_owner (msg);
  m_imap_t m_imap = msg_imap->m_imap;
  f_imap_t f_imap = m_imap->f_imap;
  struct tm tm;
  mu_timezone tz;
  time_t now;
  char datebuf[] = "mm-dd-yyyy hh:mm:ss +0000";
  const char* date = datebuf;
  const char** datep = &date;
  /* reserve as much space as we need for internal-date */
  int status;

  /* Select first.  */
  status = imap_messages_count (m_imap->mailbox, NULL);
  if (status != 0)
    return status;
  if (msg_imap->internal_date == NULL)
    {
      if (f_imap->state == IMAP_NO_STATE)
	{
	  status = imap_writeline (f_imap,
				   "g%d FETCH %d INTERNALDATE\r\n",
				   f_imap->seq++, msg_imap->num);
	  CHECK_ERROR (f_imap, status);
	  MAILBOX_DEBUG0 (m_imap->mailbox, MU_DEBUG_PROT, f_imap->buffer);
	  f_imap->state = IMAP_FETCH;
	}
      status = fetch_operation (f_imap, msg_imap, datebuf,
				sizeof datebuf, NULL);
      if (status != 0)
	return status;
      msg_imap->internal_date = strdup (datebuf);
    }
  else
    {
      date = msg_imap->internal_date;
      datep = &date;
    }

  if (mu_parse_imap_date_time(datep, &tm, &tz) != 0)
    now = (time_t)-1;
  else
    now = mu_tm2time (&tm, &tz);

  /* if the time was unparseable, or mktime() didn't like what we
     parsed, use the calendar time. */
  if (now == (time_t)-1)
    {
      struct tm* gmt;

      time(&now);
      gmt = gmtime(&now);
      tm = *gmt;
    }

  {
    int len = strftime (buffer, buflen, " %a %b %d %H:%M:%S %Y", &tm);

    /* FIXME: I don't know what strftime does if the buflen is too
       short, or it fails. Assuming that it won't fail, this is my guess
       as to the right thing.

       I think if the buffer is too short, it will fill it as much
       as it can, and nul terminate it. But I'll terminate it anyhow.
     */
    if(len == 0)
      {
	len = buflen - 1;
	buffer[len] = 0;
      }

    if(plen)
      *plen = len;
  }
  return 0;
}

/* Attributes.  */
static int
imap_attr_get_flags (attribute_t attribute, int *pflags)
{
  message_t msg = attribute_get_owner (attribute);
  msg_imap_t msg_imap = message_get_owner (msg);
  m_imap_t m_imap = msg_imap->m_imap;
  f_imap_t f_imap = m_imap->f_imap;
  int status = 0;

  /* Select first.  */
  status = imap_messages_count (m_imap->mailbox, NULL);
  if (status != 0)
    return status;

  /* Did we retrieve it alread ?  */
  if (msg_imap->flags != 0)
    {
      if (pflags)
	*pflags = msg_imap->flags;
      return 0;
    }

  if (f_imap->state == IMAP_NO_STATE)
    {
      status = imap_writeline (f_imap, "g%d FETCH %d FLAGS\r\n",
			       f_imap->seq++, msg_imap->num);
      CHECK_ERROR (f_imap, status);
      MAILBOX_DEBUG0 (m_imap->mailbox, MU_DEBUG_PROT, f_imap->buffer);
      f_imap->state = IMAP_FETCH;
    }
  status = fetch_operation (f_imap, msg_imap, NULL, 0, NULL);
  if (status == 0)
    {
      if (pflags)
	*pflags = msg_imap->flags;
    }
  return status;
}

static int
imap_attr_set_flags (attribute_t attribute, int flag)
{
  message_t msg = attribute_get_owner (attribute);
  msg_imap_t msg_imap = message_get_owner (msg);
  m_imap_t m_imap = msg_imap->m_imap;
  f_imap_t f_imap = m_imap->f_imap;
  int status = 0;

  /* Select first.  */
  status = imap_messages_count (m_imap->mailbox, NULL);
  if (status != 0)
    return status;

  /* If already set don't bother.  */
  if (msg_imap->flags & flag)
    return 0;

  /* The delete FLAG is not pass yet but only on the expunge.  */
  if (flag & MU_ATTRIBUTE_DELETED)
    {
      msg_imap->flags |= MU_ATTRIBUTE_DELETED;
      flag &= ~MU_ATTRIBUTE_DELETED;
    }

  if (f_imap->state == IMAP_NO_STATE)
    {
      char *abuf = malloc (1);
      if (abuf == NULL)
	return ENOMEM;
      *abuf = '\0';
      status = flags_to_string (&abuf, flag);
      if (status != 0)
	return status;
      /* No flags to send??  */
      if (*abuf == '\0')
	{
	  free (abuf);
	  return 0;
	}
      status = imap_writeline (f_imap, "g%d STORE %d +FLAGS.SILENT (%s)\r\n",
			       f_imap->seq++, msg_imap->num, abuf);
      free (abuf);
      CHECK_ERROR (f_imap, status);
      MAILBOX_DEBUG0 (m_imap->mailbox, MU_DEBUG_PROT, f_imap->buffer);
      msg_imap->flags |= flag;
      f_imap->state = IMAP_FETCH;
    }
  return fetch_operation (f_imap, msg_imap, NULL, 0, NULL);
}

static int
imap_attr_unset_flags (attribute_t attribute, int flag)
{
  message_t msg = attribute_get_owner (attribute);
  msg_imap_t msg_imap = message_get_owner (msg);
  m_imap_t m_imap = msg_imap->m_imap;
  f_imap_t f_imap = m_imap->f_imap;
  int status = 0;

  /* Select first.  */
  status = imap_messages_count (m_imap->mailbox, NULL);
  if (status != 0)
    return status;

  /* The delete FLAG is not pass yet but only on the expunge.  */
  if (flag & MU_ATTRIBUTE_DELETED)
    {
      msg_imap->flags &= ~MU_ATTRIBUTE_DELETED;
      flag &= ~MU_ATTRIBUTE_DELETED;
    }

  if (f_imap->state == IMAP_NO_STATE)
    {
      char *abuf = malloc (1);
      if (abuf == NULL)
	return ENOMEM;
      *abuf = '\0';
      status = flags_to_string (&abuf, flag);
      if (status != 0)
	return status;
      /* No flags to send??  */
      if (*abuf == '\0')
	{
	  free (abuf);
	  return 0;
	}
      status = imap_writeline (f_imap, "g%d STORE %d -FLAGS.SILENT (%s)\r\n",
			       f_imap->seq++, msg_imap->num, abuf);
      free (abuf);
      CHECK_ERROR (f_imap, status);
      MAILBOX_DEBUG0 (m_imap->mailbox, MU_DEBUG_PROT, f_imap->buffer);
      msg_imap->flags &= ~flag;
      f_imap->state = IMAP_FETCH;
    }
  return fetch_operation (f_imap, msg_imap, NULL, 0, NULL);
}

/* Header.  */
static int
imap_header_get_value (header_t header, const char *field, char * buffer,
		       size_t buflen, size_t *plen)
{
  message_t msg = header_get_owner (header);
  msg_imap_t msg_imap = message_get_owner (msg);
  m_imap_t m_imap = msg_imap->m_imap;
  f_imap_t f_imap = m_imap->f_imap;
  int status;
  size_t len = 0;
  char *value;

  /* Select first.  */
  status = imap_messages_count (m_imap->mailbox, NULL);
  if (status != 0)
    return status;

  /* Hack, if buffer == NULL they want to know how big is the field value,
     Unfortunately IMAP does not say, so we take a guess hoping that the
     value will not be over 1024.  */
  if (buffer == NULL || buflen == 0)
    len = 1024;
  else
    len = strlen (field) + buflen + 4;

  if (f_imap->state == IMAP_NO_STATE)
    {
      /* Select first.  */
      status = imap_messages_count (m_imap->mailbox, NULL);
      if (status != 0)
	return status;
      status = imap_writeline (f_imap,
			       "g%d FETCH %d BODY.PEEK[HEADER.FIELDS (%s)]<0.%d>\r\n",
			       f_imap->seq++, msg_imap->num, field, len);
      CHECK_ERROR (f_imap, status);
      MAILBOX_DEBUG0 (m_imap->mailbox, MU_DEBUG_PROT, f_imap->buffer);
      f_imap->state = IMAP_FETCH;

    }

  value = calloc (len, sizeof (*value));
  status = fetch_operation (f_imap, msg_imap, value, len, &len);
  if (status == 0)
    {
      char *colon;
      /* The field-matching is case-insensitive.  In all cases, the
	 delimiting newline between the header and the body is always
	 included. Nuke it  */
      if (len)
	value[len - 1] = '\0';

      /* Move pass the field-name.  */
      colon = strchr (value, ':');
      if (colon)
	{
	  colon++;
	  if (*colon == ' ')
	    colon++;
	}
      else
	colon = value;

      while (*colon && colon[strlen (colon) - 1] == '\n')
	colon[strlen (colon) - 1] = '\0';

      if (buffer && buflen)
	{
	  strncpy (buffer, colon, buflen);
	  buffer[buflen - 1] = '\0';
	}
      len = strlen (buffer);
      if (plen)
	*plen = len;
      if (len == 0)
	status = ENOENT;
    }
  free (value);
  return status;
}

static int
imap_header_get_fvalue (header_t header, const char *field, char * buffer,
			size_t buflen, size_t *plen)
{
  message_t msg = header_get_owner (header);
  msg_imap_t msg_imap = message_get_owner (msg);
  m_imap_t m_imap = msg_imap->m_imap;
  f_imap_t f_imap = m_imap->f_imap;
  int status;
  size_t len = 0;
  char *value;

  /* Select first.  */
  status = imap_messages_count (m_imap->mailbox, NULL);
  if (status != 0)
    return status;

  /* Do we all ready have the headers.  */
  if (msg_imap->fheader)
    return header_get_value (msg_imap->fheader, field, buffer, buflen, plen);

  /* We are caching the must use headers.  */
  if (f_imap->state == IMAP_NO_STATE)
    {
      /* Select first.  */
      status = imap_messages_count (m_imap->mailbox, NULL);
      if (status != 0)
        return status;
      status = imap_writeline (f_imap,
                               "g%d FETCH %d (FLAGS RFC822.SIZE BODY.PEEK[HEADER.FIELDS (%s)])\r\n",
                               f_imap->seq++, msg_imap->num,
			       MU_IMAP_CACHE_HEADERS);
      CHECK_ERROR (f_imap, status);
      MAILBOX_DEBUG0 (m_imap->mailbox, MU_DEBUG_PROT, f_imap->buffer);
      f_imap->state = IMAP_FETCH;

    }

  /* Should be enough for our needs.  */
  len = 2048;
  value = calloc (len, sizeof *value);
  status = fetch_operation (f_imap, msg_imap, value, len, &len);
  if (status == 0)
    {
      status = header_create (&msg_imap->fheader, value, len, NULL);
      if (status == 0)
	status =  header_get_value (msg_imap->fheader, field, buffer,
				    buflen, plen);
    }
  free (value);
  return status;
}

static int
imap_header_read (header_t header, char *buffer, size_t buflen, off_t offset,
		  size_t *plen)
{
  message_t msg = header_get_owner (header);
  msg_imap_t msg_imap = message_get_owner (msg);
  m_imap_t m_imap = msg_imap->m_imap;
  f_imap_t f_imap = m_imap->f_imap;
  char *oldbuf = NULL;
  char newbuf[2];
  int status;

  /* This is so annoying, a buffer len of 1 is a killer. If you have for
     example "\n" to retrieve from the server, IMAP will transform this to
     "\r\n" and since you ask for only 1, the server will send '\r' only.
     And ... '\r' will be stripped by (imap_readline()) the number of char
     read will be 0 which means we're done .... sigh ...  So we guard by at
     least ask for 2 chars.  */
  if (buflen == 1)
    {
      oldbuf = buffer;
      buffer = newbuf;
      buflen = 2;
    }

  /* Start over.  */
  if (offset == 0)
    msg_imap->header_lines = 0;

  /* Select first.  */
  status = imap_messages_count (m_imap->mailbox, NULL);
  if (status != 0)
    return status;

  if (f_imap->state == IMAP_NO_STATE)
    {
      /* We strip the \r, but the offset/size on the imap server is with that
         octet so add it in the offset, since it's the number of lines.  */
      if (msg_imap->part)
        {
          char *section = section_name (msg_imap);
          status = imap_writeline (f_imap,
                                   "g%d FETCH %d BODY.PEEK[%s.MIME]<%d.%d>\r\n",
                                   f_imap->seq++, msg_imap->num,
                                   (section) ? section : "",
                                   offset + msg_imap->header_lines, buflen);
          if (section)
            free (section);
        }
      else
        status = imap_writeline (f_imap,
                                 "g%d FETCH %d BODY.PEEK[HEADER]<%d.%d>\r\n",
                                 f_imap->seq++, msg_imap->num,
                                 offset + msg_imap->header_lines, buflen);
      CHECK_ERROR (f_imap, status);
      MAILBOX_DEBUG0 (m_imap->mailbox, MU_DEBUG_PROT, f_imap->buffer);
      f_imap->state = IMAP_FETCH;

    }
  status = fetch_operation (f_imap, msg_imap, buffer, buflen, plen);
  if (oldbuf)
    oldbuf[0] = buffer[0];
  return status;
}

/* Body.  */
static int
imap_body_size (body_t body, size_t *psize)
{
  message_t msg = body_get_owner (body);
  msg_imap_t msg_imap = message_get_owner (msg);
  if (psize && msg_imap)
    {
      /* If there is a parent it means it is a sub message, IMAP does not give
	 the full size of mime messages, so the message_size was retrieve from
	 doing a bodystructure and represents rather the body_size.  */
      if (msg_imap->parent)
	{
	  *psize = msg_imap->message_size - msg_imap->message_lines;
	}
      else
	{
	  if (msg_imap->body_size)
	    *psize = msg_imap->body_size;
	  else if (msg_imap->message_size)
	    *psize = msg_imap->message_size
	      - (msg_imap->header_size + msg_imap->header_lines);
	  else
	    *psize = 0;
	}
    }
  return 0;
}

static int
imap_body_lines (body_t body, size_t *plines)
{
  message_t msg = body_get_owner (body);
  msg_imap_t msg_imap = message_get_owner (msg);
  if (plines && msg_imap)
    *plines = msg_imap->body_lines;
  return 0;
}

/* FIXME: Send EISPIPE if trying to seek back.  */
static int
imap_body_read (stream_t stream, char *buffer, size_t buflen, off_t offset,
		size_t *plen)
{
  body_t body = stream_get_owner (stream);
  message_t msg = body_get_owner (body);
  msg_imap_t msg_imap = message_get_owner (msg);
  m_imap_t m_imap = msg_imap->m_imap;
  f_imap_t f_imap = m_imap->f_imap;
  char *oldbuf = NULL;
  char newbuf[2];
  int status;

  /* This is so annoying, a buffer len of 1 is a killer. If you have for
     example "\n" to retrieve from the server, IMAP will transform this to
     "\r\n" and since you ask for only 1, the server will send '\r' only.
     And ... '\r' will be stripped by (imap_readline()) the number of char
     read will be 0 which means we're done .... sigh ...  So we guard by at
     least ask for 2 chars.  */
  if (buflen == 1)
    {
      oldbuf = buffer;
      buffer = newbuf;
      buflen = 2;
    }

  /* Start over.  */
  if (offset == 0)
    {
      msg_imap->body_lines = 0;
      msg_imap->body_size = 0;
    }

  /* Select first.  */
  status = imap_messages_count (m_imap->mailbox, NULL);
  if (status != 0)
    return status;

  if (f_imap->state == IMAP_NO_STATE)
    {
      /* We strip the \r, but the offset/size on the imap server is with the
         octet, so add it since it's the number of lines.  */
      if (msg_imap->part)
        {
          char *section = section_name (msg_imap);
          status = imap_writeline (f_imap,
                                   "g%d FETCH %d BODY.PEEK[%s]<%d.%d>\r\n",
                                   f_imap->seq++, msg_imap->num,
                                   (section) ? section: "",
                                   offset + msg_imap->body_lines, buflen);
          if (section)
            free (section);
        }
      else
        status = imap_writeline (f_imap,
                                 "g%d FETCH %d BODY.PEEK[TEXT]<%d.%d>\r\n",
                                 f_imap->seq++, msg_imap->num,
                                 offset + msg_imap->body_lines, buflen);
      CHECK_ERROR (f_imap, status);
      MAILBOX_DEBUG0 (m_imap->mailbox, MU_DEBUG_PROT, f_imap->buffer);
      f_imap->state = IMAP_FETCH;

    }
  status = fetch_operation (f_imap, msg_imap, buffer, buflen, plen);
  if (oldbuf)
    oldbuf[0] = buffer[0];
  return status;
}

static int
imap_body_fd (stream_t stream, int *pfd, int *pfd2)
{
  body_t body = stream_get_owner (stream);
  message_t msg = body_get_owner (body);
  msg_imap_t msg_imap = message_get_owner (msg);
  return imap_get_fd2 (msg_imap, pfd, pfd2);
}


static int
imap_get_fd2 (msg_imap_t msg_imap, int *pfd1, int *pfd2)
{
  if (   msg_imap
      && msg_imap->m_imap
      && msg_imap->m_imap->f_imap
      && msg_imap->m_imap->f_imap->folder)
    return stream_get_fd2 (msg_imap->m_imap->f_imap->folder->stream,
			   pfd1, pfd2);
  return EINVAL;
}

/* Since so many operations are fetch, we regoup this into one function.  */
static int
fetch_operation (f_imap_t f_imap, msg_imap_t msg_imap, char *buffer,
		 size_t buflen, size_t *plen)
{
  int status = 0;

  switch (f_imap->state)
    {
    case IMAP_FETCH:
      status = imap_send (f_imap);
      CHECK_EAGAIN (f_imap, status);
      stream_truncate (f_imap->string.stream, 0);
      f_imap->string.offset = 0;
      f_imap->string.nleft = 0;
      f_imap->string.type = IMAP_NO_STATE;
      f_imap->string.msg_imap = msg_imap;
      f_imap->state = IMAP_FETCH_ACK;

    case IMAP_FETCH_ACK:
      status = imap_parse (f_imap);
      CHECK_EAGAIN (f_imap, status);
      if (f_imap->selected)
	MAILBOX_DEBUG0 (f_imap->selected->mailbox, MU_DEBUG_PROT,
			f_imap->buffer);

    default:
      break;
    }

  f_imap->state = IMAP_NO_STATE;

  /* The server may have timeout any case connection is gone away.  */
  if (status == 0 && f_imap->isopen == 0 && f_imap->string.offset == 0)
    status = EBADF;

  if (buffer)
    stream_read (f_imap->string.stream, buffer, buflen, 0, plen);
  else if (plen)
    *plen = 0;
  stream_truncate (f_imap->string.stream, 0);
  f_imap->string.offset = 0;
  f_imap->string.nleft = 0;
  f_imap->string.type = IMAP_NO_STATE;
  f_imap->string.msg_imap = NULL;
  return status;
}

/* Decide whether the message came from the same folder as the mailbox.  */
static int
is_same_folder (mailbox_t mailbox, message_t msg)
{
  mailbox_t mbox = NULL;
  message_get_mailbox (msg, &mbox);
  return (mbox != NULL && mbox->url != NULL
          && url_is_same_scheme (mbox->url, mailbox->url)
          && url_is_same_host (mbox->url, mailbox->url)
          && url_is_same_port (mbox->url, mailbox->url));
}

/* Convert flag attribute to IMAP String attributes.  */
static int
flags_to_string (char **pbuf, int flag)
{
  char *abuf = *pbuf;
  if (flag & MU_ATTRIBUTE_DELETED)
    {
      char *tmp = realloc (abuf, strlen (abuf) + strlen ("\\Deleted") + 2);
      if (tmp == NULL)
        {
          free (abuf);
          return ENOMEM;
        }
      abuf = tmp;
      if (*abuf)
        strcat (abuf, " ");
      strcat (abuf, "\\Deleted");
    }
  if (flag & MU_ATTRIBUTE_READ)
    {
      char *tmp = realloc (abuf, strlen (abuf) + strlen ("\\Seen") + 2);
      if (tmp == NULL)
        {
          free (abuf);
          return ENOMEM;
        }
      abuf = tmp;
      if (*abuf)
        strcat (abuf, " ");
      strcat (abuf, "\\Seen");
    }
  if (flag & MU_ATTRIBUTE_ANSWERED)
    {
      char *tmp = realloc (abuf, strlen (abuf) + strlen ("\\Answered") + 2);
      if (tmp == NULL)
        {
          free (abuf);
          return ENOMEM;
        }
      abuf = tmp;
      if (*abuf)
        strcat (abuf, " ");
      strcat (abuf, "\\Answered");
    }
  if (flag & MU_ATTRIBUTE_DRAFT)
    {
      char *tmp = realloc (abuf, strlen (abuf) + strlen ("\\Draft") + 2);
      if (tmp == NULL)
        {
          free (abuf);
          return ENOMEM;
        }
      abuf = tmp;
      if (*abuf)
        strcat (abuf, " ");
      strcat (abuf, "\\Draft");
    }
  if (flag & MU_ATTRIBUTE_FLAGGED)
    {
      char *tmp = realloc (abuf, strlen (abuf) + strlen ("\\Flagged") + 2);
      if (tmp == NULL)
        {
          free (abuf);
          return ENOMEM;
        }
      abuf = tmp;
      if (*abuf)
        strcat (abuf, " ");
      strcat (abuf, "\\Flagged");
    }
  *pbuf = abuf;
  return 0;
}

/* Convert a suite of number to IMAP message number.  */
static int
add_number (char **pset, size_t start, size_t end)
{
  char buf[128];
  char *set;
  char *tmp;
  size_t set_len = 0;

  if (pset == NULL)
    return 0;

  set = *pset;

  if (set)
    set_len = strlen (set);

  /* We had a previous seqence.  */
  if (start == 0)
    *buf = '\0';
  else if (start != end)
    snprintf (buf, sizeof buf, "%lu:%lu",
	      (unsigned long) start,
	      (unsigned long) end);
  else
    snprintf (buf, sizeof buf, "%lu", (unsigned long) start);

  if (set_len)
    tmp = realloc (set, set_len + strlen (buf) + 2 /* null and comma */);
  else
    tmp = calloc (strlen (buf) + 1, 1);

  if (tmp == NULL)
    {
      free (set);
      return ENOMEM;
    }
  set = tmp;

  /* If we had something add a comma separator.  */
  if (set_len)
    strcat (set, ",");
  strcat (set, buf);

  *pset = set;
  return 0;
}

static int
delete_to_string (m_imap_t m_imap, char **pset)
{
  int status;
  size_t i, prev = 0, is_range = 0;
  size_t start = 0, cur = 0;
  char *set = NULL;

  /* Reformat the number for IMAP.  */
  for (i = 0; i < m_imap->imessages_count; ++i)
    {
      if (m_imap->imessages[i]
	  && (m_imap->imessages[i]->flags & MU_ATTRIBUTE_DELETED))
	{
	  cur = m_imap->imessages[i]->num;
	  /* The first number.  */
	  if (start == 0)
	    {
	      start = prev = cur;
	    }
	  /* Is it a sequence?  */
	  else if ((prev + 1) == cur)
	    {
	      prev = cur;
	      is_range = 1;
	    }
	  continue;
	}

      if (start)
	{
	  status = add_number (&set, start, cur);
	  if (status != 0)
	    return status;
	  start = 0;
	  prev = 0;
	  cur = 0;
	  is_range = 0;
	}
    } /* for () */

  status = add_number (&set, start, cur);
  if (status != 0)
    return status;
  *pset = set;
  return 0;
}

#endif
