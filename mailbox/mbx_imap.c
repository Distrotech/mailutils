/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU Library General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>

#include <mailutils/address.h>
#include <mailbox0.h>
#include <registrar0.h>
#include <imap0.h>

/* Functions to overload the mailbox_t API.  */
static void mailbox_imap_destroy (mailbox_t);
static int mailbox_imap_open (mailbox_t, int);
static int mailbox_imap_close (mailbox_t);
static int imap_uidvalidity (mailbox_t, unsigned long *);
static int imap_uidnext (mailbox_t, size_t *);
static int imap_expunge (mailbox_t);
static int imap_get_message (mailbox_t, size_t, message_t *);
static int imap_messages_count (mailbox_t, size_t *);
static int imap_messages_recent (mailbox_t, size_t *);
static int imap_message_unseen (mailbox_t, size_t *);
static int imap_scan (mailbox_t, size_t, size_t *);
static int imap_is_updated (mailbox_t);
static int imap_append_message (mailbox_t, message_t);

/* Message API.  */

static int imap_message_size (message_t, size_t *);
static int imap_message_lines (message_t, size_t *);
static int imap_message_fd (stream_t, int *);
static int imap_message_read (stream_t , char *, size_t, off_t, size_t *);
static int imap_message_uid (message_t, size_t *);

/* Mime handling.  */
static int imap_is_multipart (message_t, int *);
static int imap_get_num_parts (message_t, size_t *);
static int imap_get_part (message_t, size_t, message_t *);

/* Envelope.  */
static int imap_envelope_sender (envelope_t, char *, size_t, size_t *);
static int imap_envelope_date (envelope_t, char *, size_t, size_t *);

/* Attributes.  */
static int imap_attr_get_flags (attribute_t, int *);
static int imap_attr_set_flags (attribute_t, int);
static int imap_attr_unset_flags (attribute_t, int);

/* Header.  */
static int imap_header_read (header_t, char*, size_t, off_t, size_t *);
static int imap_header_get_value (header_t, const char*, char *, size_t, size_t *);

/* Body.  */
static int imap_body_read (stream_t, char *, size_t, off_t, size_t *);
static int imap_body_size (body_t, size_t *);
static int imap_body_lines (body_t, size_t *);
static int imap_body_fd (stream_t, int *);

/* Private. */
static int imap_get_fd (msg_imap_t, int *);
static int imap_get_message0 (msg_imap_t, message_t *);
static int message_operation (f_imap_t, msg_imap_t, enum imap_state, char *,
			      size_t, size_t *);
static void free_subparts (msg_imap_t);

static const char *MONTHS[] =
{
  "Jan", "Feb", "Mar", "Apr", "May", "Jun",
  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};


/* Initialize the concrete object mailbox_t by overloading the function of the
   structure.  */
int
_mailbox_imap_init (mailbox_t mailbox)
{
  m_imap_t m_imap;
  size_t name_len = 0;
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

  return 0;
}

/* Recursive call to free all the subparts of a message.  */
static void
free_subparts (msg_imap_t msg_imap)
{
  size_t i;
  for (i = 0; i < msg_imap->num_parts; i++)
    if (msg_imap->parts[i])
      free_subparts (msg_imap->parts[i]);

  if (msg_imap->message)
    message_destroy (&(msg_imap->message), msg_imap);
  if (msg_imap->parts)
    free (msg_imap->parts);
  free(msg_imap);
}

/* Give back all the resources. But is does not mean to shutdown the channel
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
	if (m_imap->imessages[i])
	  free_subparts (m_imap->imessages[i]);
      if (m_imap->imessages)
	free (m_imap->imessages);
      if (m_imap->name)
	free (m_imap->name);
      free (m_imap);
      mailbox->data = NULL;
      monitor_unlock (mailbox->monitor);
    }
}

/* If the connection was not up it is open by the folder.  It is not necessary
   to set this mailbox selected on the folder, there maybe on going operation.
   But on any operation, if is not selected will send the SELECT.  */
static int
mailbox_imap_open (mailbox_t mailbox, int flags)
{
  return folder_open (mailbox->folder, flags);
}

/* We can not close the folder in term of shuting down the connection but if
   we were the selected mailbox we send the close and deselect ourself.  */
static int
mailbox_imap_close (mailbox_t mailbox)
{
  m_imap_t m_imap = mailbox->data;
  f_imap_t f_imap = m_imap->f_imap;
  int status = 0;

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
      MAILBOX_DEBUG0 (mailbox, MU_DEBUG_PROT, f_imap->buffer);
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
	  if (m_imap->imessages[i])
	    free_subparts (m_imap->imessages[i]);
	if (m_imap->imessages)
	  free (m_imap->imessages);
	m_imap->imessages = NULL;
	m_imap->imessages_count = 0;
	monitor_unlock (mailbox->monitor);
      }

    default:
      break;
    }

  /* Deselect.  */
  f_imap->selected = NULL;

  f_imap->state = IMAP_NO_STATE;
  return status;
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
  size_t i;

  if (pmsg == NULL)
    return EINVAL;

  monitor_rdlock (mailbox->monitor);
  /* See if we have already this message.  */
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
  monitor_unlock (mailbox->monitor);

  /* Allocate a concrete imap message.  */
  msg_imap = calloc (1, sizeof (*msg_imap));
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
		     (m_imap->imessages_count + 1) * sizeof (*m));
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
      msg_imap->m_imap = m_imap;
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

  /* Create the message.  */
  {
    stream_t stream = NULL;
    if ((status = message_create (&msg, msg_imap)) != 0
        || (status = stream_create (&stream, mailbox->flags, msg)) != 0)
      {
        stream_destroy (&stream, msg);
        message_destroy (&msg, msg_imap);
        return status;
      }
    /* We want the buffering.  */
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
    message_set_header (msg, header, msg_imap);
  }

  /* We do not create any special attribute, since nothing is send to the
     server.  When the attributes change they are cache, they are only
     sent to the server on mailbox_close or mailbox_expunge.  */
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
    /* We want the buffering.  */
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
   back on other server response as an untag "EXIST" response.  But we still
   send a SELECT.  */
static int
imap_messages_count (mailbox_t mailbox, size_t *pnum)
{
  m_imap_t m_imap = mailbox->data;
  f_imap_t f_imap = m_imap->f_imap;
  int status = 0;

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
      MAILBOX_DEBUG0 (mailbox, MU_DEBUG_PROT, f_imap->buffer);
      if (status != 0)
	f_imap->selected = NULL;
      f_imap->state = IMAP_SELECT_ACK;

    case IMAP_SELECT_ACK:
      status = imap_parse (f_imap);
      CHECK_EAGAIN (f_imap, status);
      MAILBOX_DEBUG0 (mailbox, MU_DEBUG_PROT, f_imap->buffer);

    default:
      break;
    }

  if (pnum)
    *pnum = m_imap->messages_count;

  f_imap->state = IMAP_NO_STATE;
  return status;
}

/* We simulate by sending a notification for the total of msgno.  */
static int
imap_scan (mailbox_t mailbox, size_t msgno, size_t *pcount)
{
  int status;
  size_t i;
  size_t count = 0;

  /* Selected.  */
  status = imap_messages_count (mailbox, &count);
  if (pcount)
    *pcount = count;
  if (status != 0)
    return status;
  if (mailbox->observable == NULL)
    return 0;
  for (i = msgno; i <= *pcount; i++)
    if (observable_notify (mailbox->observable, MU_EVT_MESSAGE_ADD) != 0)
      break;
  return 0;
}

/* Send a NOOP and see if the count has change.  */
static int
imap_is_updated (mailbox_t mailbox)
{
  m_imap_t m_imap = mailbox->data;
  size_t oldcount = m_imap->messages_count;
  f_imap_t f_imap = m_imap->f_imap;
  int status = 0;

  /* Send a noop, and let imap piggy pack the information.  */
  switch (f_imap->state)
    {
    case IMAP_NO_STATE:
      /* Selected.  */
      status = imap_messages_count (mailbox, &oldcount);
      if (status != 0)
	return status;
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

    default:
      break;
    }
  f_imap->state = IMAP_NO_STATE;
  return (oldcount == m_imap->messages_count);
}


/* FIXME: Not asyn, please fix to make usable when non blocking.  */
static int
imap_expunge (mailbox_t mailbox)
{
  size_t i;
  int status;
  m_imap_t m_imap = mailbox->data;
  f_imap_t f_imap = m_imap->f_imap;

  /* Select first.  */
  status = imap_messages_count (mailbox, &i);
  if (status != 0)
    return status;

  for (i = 0; i < m_imap->imessages_count; ++i)
    {
      if (m_imap->imessages[i]->flags & MU_ATTRIBUTE_DELETED)
	{
	  switch (f_imap->state)
	    {
	    case IMAP_NO_STATE:
	      status = imap_writeline (f_imap,
				       "g%d STORE %d +FLAGS.SILENT (\\Deleted)\r\n",
				       f_imap->seq++,
				       m_imap->imessages[i]->num);
	      CHECK_ERROR (f_imap, status);
	      MAILBOX_DEBUG0 (m_imap->mailbox, MU_DEBUG_PROT, f_imap->buffer);
	      f_imap->state = IMAP_STORE;

	    case IMAP_STORE:
	      /* Send DELETE.  */
	      status = imap_send (f_imap);
	      CHECK_EAGAIN (f_imap, status);
	      f_imap->state = IMAP_STORE_ACK;

	    case IMAP_STORE_ACK:
	      status = imap_parse (f_imap);
	      CHECK_EAGAIN (f_imap, status);
	      MAILBOX_DEBUG0 (m_imap->mailbox, MU_DEBUG_PROT, f_imap->buffer);
	      f_imap->state = IMAP_NO_STATE;

	    default:
	      /* fprintf (stderr, "imap_expunge: unknow state\n"); */
	      break;
	    } /* switch (state) */
        } /* message_get_attribute() */
    } /* for */

  /* Tell the server to delete the messages but without sending the
     EXPUNGE response.  We can do the calculations.  */
  status = mailbox_imap_close (mailbox);
  return status;
}

static int
imap_append_message (mailbox_t mailbox, message_t msg)
{
  (void)mailbox; (void)msg;
  return 0;
}

/* Message.  */
static int
imap_message_read (stream_t stream, char *buffer, size_t buflen,
		   off_t offset, size_t *plen)
{
  message_t msg = stream_get_owner (stream);
  msg_imap_t msg_imap = message_get_owner (msg);
  m_imap_t m_imap = msg_imap->m_imap;
  f_imap_t f_imap = m_imap->f_imap;

  /* Select first.  */
  if (f_imap->state == IMAP_NO_STATE)
    {
      int status = imap_messages_count (m_imap->mailbox, NULL);
      if (status != 0)
	return status;
      /* We strip the \r, but the offset/size on the imap server is with that
	 octet so add it in the offset, since it's the number of lines.  */
      if (msg_imap->part)
	{
	  char *section = section_name (msg_imap);
	  status = imap_writeline (f_imap,
				   "g%d FETCH %d BODY.PEEK[%s]<%d.%d>\r\n",
				   f_imap->seq++, msg_imap->num,
				   (section) ? section : "",
				   offset + msg_imap->message_lines, buflen);
	  if (section)
	    free (section);
	}
      else
	status = imap_writeline (f_imap,
				 "g%d FETCH %d BODY.PEEK[]<%d.%d>\r\n",
				 f_imap->seq++, msg_imap->num,
				 offset + msg_imap->message_lines, buflen);
      CHECK_ERROR (f_imap, status);
      MAILBOX_DEBUG0 (m_imap->mailbox, MU_DEBUG_PROT, f_imap->buffer);
      f_imap->state = IMAP_FETCH;
    }
  return message_operation (f_imap, msg_imap, IMAP_MESSAGE, buffer, buflen,
			    plen);
}

static int
imap_message_lines (message_t msg, size_t *plines)
{
  msg_imap_t msg_imap = message_get_owner (msg);
  if (plines && msg_imap)
    *plines = msg_imap->message_lines;
  return 0;
}

static int
imap_message_size (message_t msg, size_t *psize)
{
  msg_imap_t msg_imap = message_get_owner (msg);
  m_imap_t m_imap = msg_imap->m_imap;
  f_imap_t f_imap = m_imap->f_imap;
  int status;

  /* Select first.  */
  if (f_imap->state == IMAP_NO_STATE)
    {
      status = imap_messages_count (m_imap->mailbox, NULL);
      if (status != 0)
	return status;
      /* We strip the \r, but the offset/size on the imap server is with that
	 octet so add it in the offset, since it's the number of lines.  */
      status = imap_writeline (f_imap,
			       "g%d FETCH %d RFC822.SIZE\r\n",
			       f_imap->seq++, msg_imap->num);
      CHECK_ERROR (f_imap, status);
      MAILBOX_DEBUG0 (m_imap->mailbox, MU_DEBUG_PROT, f_imap->buffer);
      f_imap->state = IMAP_FETCH;
    }
  status = message_operation (f_imap, msg_imap, 0, 0, 0, 0);
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
  if (f_imap->state == IMAP_NO_STATE)
    {
      if (msg_imap->uid)
	{
	  *puid = msg_imap->uid;
	  return 0;
	}
      status = imap_messages_count (m_imap->mailbox, NULL);
      if (status != 0)
	return status;
      status = imap_writeline (f_imap, "g%d FETCH %d UID\r\n",
			       f_imap->seq++, msg_imap->num);
      CHECK_ERROR (f_imap, status);
      MAILBOX_DEBUG0 (m_imap->mailbox, MU_DEBUG_PROT, f_imap->buffer);
      f_imap->state = IMAP_FETCH;
    }
  status = message_operation (f_imap, msg_imap, 0, 0, 0, 0);
  if (status != 0)
    return status;
  *puid = msg_imap->uid;
  return 0;
}

static int
imap_message_fd (stream_t stream, int * pfd)
{
  message_t msg = stream_get_owner (stream);
  msg_imap_t msg_imap = message_get_owner (msg);
  return imap_get_fd (msg_imap, pfd);
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
  if (f_imap->state == IMAP_NO_STATE)
    {
      if (msg_imap->num_parts || msg_imap->part)
	{
	  if (ismulti)
	    *ismulti = (msg_imap->num_parts > 1);
	  return 0;
	}
      status = imap_messages_count (m_imap->mailbox, NULL);
      if (status != 0)
	return status;
      status = imap_writeline (f_imap,
			       "g%d FETCH %d BODY\r\n",
			       f_imap->seq++, msg_imap->num);
      CHECK_ERROR (f_imap, status);
      MAILBOX_DEBUG0 (m_imap->mailbox, MU_DEBUG_PROT, f_imap->buffer);
      f_imap->state = IMAP_FETCH;
    }
  status = message_operation (f_imap, msg_imap, 0, 0, 0, 0);
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
	      message_set_size (message, NULL, msg_imap->parts[partno - 1]);
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

  message_get_header (msg, &header);
  status = imap_header_get_value (header, MU_HEADER_SENDER, buffer,
					  buflen, plen);
  if (status == EAGAIN)
    return status;
  else if (status != 0)
    status = imap_header_get_value (header, MU_HEADER_FROM, buffer,
					    buflen, plen);
  if (status == 0)
    {
      address_t address;
      if (address_create (&address, buffer) == 0)
	{
	  address_get_email (address, 1, buffer, buflen, plen);
	  address_destroy (&address);
	}
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
  int year, mon, day, hour, min, sec;
  int offt;
  int i;
  struct tm tm;
  time_t now;
  char month[5];
  int status;
  if (f_imap->state == IMAP_NO_STATE)
    {
      /* Select first.  */
      status = imap_messages_count (m_imap->mailbox, NULL);
      if (status != 0)
	return status;
      status = imap_writeline (f_imap,
			       "g%d FETCH %d INTERNALDATE\r\n",
			       f_imap->seq++, msg_imap->num);
      CHECK_ERROR (f_imap, status);
      MAILBOX_DEBUG0 (m_imap->mailbox, MU_DEBUG_PROT, f_imap->buffer);
      f_imap->state = IMAP_FETCH;
    }
  status = message_operation (f_imap, msg_imap, 0, buffer, buflen, plen);
  if (status != 0)
    return status;
  day = mon = year = hour = min = sec = offt = 0;
  month[0] = '\0';
  sscanf (buffer, "%2d-%3s-%4d %2d:%2d:%2d %d", &day, month, &year,
	  &hour, &min, &sec, &offt);
  tm.tm_sec = sec;
  tm.tm_min = min;
  tm.tm_hour = hour;
  tm.tm_mday = day;
  for (i = 0; i < 12; i++)
    {
      if (strncasecmp(month, MONTHS[i], 3) == 0)
	{
	  mon = i;
	  break;
	}
    }
  tm.tm_mon = mon;
  tm.tm_year = (year > 1900) ? year - 1900 : year;
  tm.tm_yday = 0; /* unknown. */
  tm.tm_wday = 0; /* unknown. */
  tm.tm_isdst = -1; /* unknown. */
  /* What to do the timezone?  */

  now = mktime (&tm);
  if (now == (time_t)-1)
    {
      /* Fall back to localtime.  */
      now = time (NULL);
      snprintf (buffer, buflen, "%s", ctime(&now));
    }
  else
    {
      strftime (buffer, buflen, " %a %b %d %H:%M:%S %Y", &tm);
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
  status = message_operation (f_imap, msg_imap, 0, NULL, 0, NULL);
  if (status == 0)
    {
      if (pflags)
	*pflags = msg_imap->flags;
    }
  return status;
}

static int
imap_attr_set_flags (attribute_t attribute, int flags)
{
  message_t msg = attribute_get_owner (attribute);
  msg_imap_t msg_imap = message_get_owner (msg);
  m_imap_t m_imap = msg_imap->m_imap;
  f_imap_t f_imap = m_imap->f_imap;
  int status = 0;

  /* The delete FLAG is not pass yet but only on the expunge.  */
  if (f_imap->state == IMAP_NO_STATE)
    {
      status = imap_writeline (f_imap, "g%d STORE %d +FLAGS.SILENT (%s %s %s %s)\r\n",
			       f_imap->seq++, msg_imap->num,
			       (flags & MU_ATTRIBUTE_SEEN) ? "\\Seen" : "",
			       (flags & MU_ATTRIBUTE_ANSWERED) ? "\\Answered" : "",
			       (flags & MU_ATTRIBUTE_DRAFT) ? "\\Draft" : "",
			       (flags & MU_ATTRIBUTE_FLAGGED) ? "\\Flagged" : "");
      CHECK_ERROR (f_imap, status);
      MAILBOX_DEBUG0 (m_imap->mailbox, MU_DEBUG_PROT, f_imap->buffer);
      msg_imap->flags |= flags;
      f_imap->state = IMAP_FETCH;
    }
  return message_operation (f_imap, msg_imap, 0, NULL, 0, NULL);
}

static int
imap_attr_unset_flags (attribute_t attribute, int flags)
{
  message_t msg = attribute_get_owner (attribute);
  msg_imap_t msg_imap = message_get_owner (msg);
  m_imap_t m_imap = msg_imap->m_imap;
  f_imap_t f_imap = m_imap->f_imap;
  int status = 0;

  if (f_imap->state == IMAP_NO_STATE)
    {
      status = imap_writeline (f_imap, "g%d STORE %d -FLAGS.SILENT (%s %s %s %s)\r\n",
			       f_imap->seq++, msg_imap->num,
			       (flags & MU_ATTRIBUTE_SEEN) ? "\\Seen" : "",
			       (flags & MU_ATTRIBUTE_ANSWERED) ? "\\Answered" : "",
			       (flags & MU_ATTRIBUTE_DRAFT) ? "\\Draft" : "",
			       (flags & MU_ATTRIBUTE_FLAGGED) ? "\\Flagged" : "");
      CHECK_ERROR (f_imap, status);
      MAILBOX_DEBUG0 (m_imap->mailbox, MU_DEBUG_PROT, f_imap->buffer);
      msg_imap->flags &= ~flags;
      f_imap->state = IMAP_FETCH;
    }
  return message_operation (f_imap, msg_imap, 0, NULL, 0, NULL);
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

  /* Hack, if buffer == NULL they want to know how big is the field value,
     Unfortunately IMAP does not say, so we take a guess hoping that the
     value will not be over 1024.  */
  /* FIXME: This is stupid, find a way to fix this.  */
  if (buffer == NULL || buflen == 0)
    len = 1024;
  else
    len = strlen (field) + buflen + 4;
  value = alloca (len);
  /*memset (value, '0', len); */

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
  status = message_operation (f_imap, msg_imap, IMAP_HEADER_FIELD, value, len,
			      &len);
  if (status == 0)
    {
      char *colon;
      /* The field-matching is case-insensitive but otherwise exact.  In all
	 cases, the delimiting blank line between the header and the body is
	 always included.  */
      value[len - 1] = '\0';
      colon = strchr (value, ':');
      if (colon)
	{
	  colon++;
	  if (*colon == ' ')
	    colon++;
	}
      else
	colon = value;

      while (colon[strlen (colon) - 1] == '\n')
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

  /* Select first.  */
  if (f_imap->state == IMAP_NO_STATE)
    {
      int status = imap_messages_count (m_imap->mailbox, NULL);
      if (status != 0)
	return status;
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
  return message_operation (f_imap, msg_imap, IMAP_HEADER, buffer, buflen,
			    plen);
}

/* Body.  */
static int
imap_body_size (body_t body, size_t *psize)
{
  message_t msg = body_get_owner (body);
  msg_imap_t msg_imap = message_get_owner (msg);
  if (psize && msg_imap)
    *psize = msg_imap->body_size;
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
  int status = 0;

  /* This is so annoying, a buffer len of 1 is a killer. If you have for
     example "\n" to retrieve from the server, IMAP will transform this to
     "\r\n" and since you ask for only 1, the server will send '\r' only.
     And ... '\r' will be stripped by (imap_readline()) the number of char
     read will be 0 which means we're done .... sigh ...  So we guard to at
     least ask for 2 chars.  */
  if (buflen == 1)
    {
      oldbuf = buffer;
      buffer = newbuf;
      buflen = 2;
    }
  /* Select first.  */
  if (f_imap->state == IMAP_NO_STATE)
    {
      status = imap_messages_count (m_imap->mailbox, NULL);
      if (status != 0)
	return status;
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
  status = message_operation (f_imap, msg_imap, IMAP_BODY, buffer, buflen, plen);
  if (oldbuf)
    oldbuf[0] = buffer[0];
  return status;
}

static int
imap_body_fd (stream_t stream, int *pfd)
{
  body_t body = stream_get_owner (stream);
  message_t msg = body_get_owner (body);
  msg_imap_t msg_imap = message_get_owner (msg);
  return imap_get_fd (msg_imap, pfd);
}


static int
imap_get_fd (msg_imap_t msg_imap, int *pfd)
{
  if (   msg_imap
      && msg_imap->m_imap
      && msg_imap->m_imap->f_imap
      && msg_imap->m_imap->f_imap->folder)
    return stream_get_fd (msg_imap->m_imap->f_imap->folder->stream, pfd);
  return EINVAL;
}

static int
message_operation (f_imap_t f_imap, msg_imap_t msg_imap, enum imap_state type,
		   char *buffer, size_t buflen, size_t *plen)
{
  int status = 0;

  switch (f_imap->state)
    {
    case IMAP_FETCH:
      status = imap_send (f_imap);
      CHECK_EAGAIN (f_imap, status);
      /* Set the callback,  we want the results.  */
      f_imap->callback.buffer = buffer;
      f_imap->callback.buflen = buflen;
      f_imap->callback.total = 0;
      f_imap->callback.type = type;
      f_imap->callback.msg_imap = msg_imap;
      f_imap->state = IMAP_FETCH_ACK;

    case IMAP_FETCH_ACK:
      status = imap_parse (f_imap);
      CHECK_EAGAIN (f_imap, status);
      MAILBOX_DEBUG0 (f_imap->selected->mailbox, MU_DEBUG_PROT,
		      f_imap->buffer);

    default:
      break;
    }
  if (plen)
    *plen = f_imap->callback.total;
  /* Clear the callback.  */
  f_imap->callback.buffer = NULL;
  f_imap->callback.buflen = 0;
  f_imap->callback.total = 0;
  f_imap->callback.type = 0;
  f_imap->callback.msg_imap = NULL;
  f_imap->state = IMAP_NO_STATE;
  return status;
}
