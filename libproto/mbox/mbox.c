/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2001, 2003-2012 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library.  If not, see
   <http://www.gnu.org/licenses/>. */

/* First draft by Alain Magloire.
 * Completely rewritten by Sergey Poznyakoff.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <mbox0.h>
#include <mailutils/cstr.h>
#include <mailutils/io.h>
#include <mailutils/filter.h>
#include <mailutils/cctype.h>
#include <mailutils/datetime.h>

#define ATTRIBUTE_IS_DELETED(flag)        (flag & MU_ATTRIBUTE_DELETED)
#define ATTRIBUTE_IS_EQUAL(flag1, flag2)  (flag1 == flag2)

static int mbox_is_updated (mu_mailbox_t mailbox);
static int mbox_expunge0 (mu_mailbox_t mailbox, int remove_deleted);

/* Free all ressources associated with Unix concrete mailbox.  */
static void
mbox_destroy (mu_mailbox_t mailbox)
{
  if (mailbox->data)
    {
      size_t i;
      mbox_data_t mud = mailbox->data;
      mu_debug (MU_DEBCAT_MAILBOX, MU_DEBUG_TRACE1,
		("mbox_destroy (%s)", mud->name));
      mu_monitor_wrlock (mailbox->monitor);
      for (i = 0; i < mud->umessages_count; i++)
	{
	  mbox_message_t mum = mud->umessages[i];
	  if (mum)
	    {
	      mu_message_destroy (&mum->message, mum);
	      free (mum);
	    }
	}
      if (mud->umessages)
	free (mud->umessages);
      if (mud->name)
	free (mud->name);
      free (mud);
      mailbox->data = NULL;
      mu_monitor_unlock (mailbox->monitor);
    }
}

/* Open the file.  For MU_STREAM_READ, the code tries mmap() first and fall
   back to normal file.  */
static int
mbox_open (mu_mailbox_t mailbox, int flags)
{
  mbox_data_t mud = mailbox->data;
  int status = 0;

  if (mud == NULL)
    return EINVAL;

  mailbox->flags = flags;

  /* Get a stream.  */
  if (mailbox->stream == NULL)
    {
      /* We do not try to mmap for CREAT or APPEND, it is not supported.  */
      status = (flags & MU_STREAM_CREAT)
	          || (mailbox->flags & MU_STREAM_APPEND);

      /* Try to mmap () the file first.  */
      if (status == 0)
	{
	  status = mu_mapfile_stream_create (&mailbox->stream, mud->name,
					     mailbox->flags);
	}

      /* Fall back to normal file if mmap() failed.  */
      if (status)
	status = mu_file_stream_create (&mailbox->stream, mud->name,
					mailbox->flags);

      if (status)
	return status;
    }

  mu_stream_set_buffer (mailbox->stream, mu_buffer_full, 0);

  mu_debug (MU_DEBCAT_MAILBOX, MU_DEBUG_TRACE1, 
            ("mbox_open (%s, 0x%x)", mud->name, mailbox->flags));

  if (mailbox->locker == NULL &&
      (flags & (MU_STREAM_WRITE | MU_STREAM_APPEND | MU_STREAM_CREAT)))
    status = mu_locker_create (&mailbox->locker, mud->name, 0);
  return status;
}

static int
mbox_close (mu_mailbox_t mailbox)
{
  mbox_data_t mud = mailbox->data;
  size_t i; 

  if (mud == NULL)
    return EINVAL;

  mu_debug (MU_DEBCAT_MAILBOX, MU_DEBUG_TRACE1,
	    ("mbox_close (%s)", mud->name));

  /* Make sure that we do not hold any file locking.  */
  mu_locker_unlock (mailbox->locker);

  /* Alain: I'm not sure on the right approach especially if the client is
     working in disconnected mode, where it can mu_mailbox_close/
     mu_mailbox_open for each request, maybe we should keep them for a while.

     Sergey: No, it actually breaks reopening the mailbox. We should make
     sure that the sequence mu_mailbox_open();mu_mailbox_close() catches all
     the changes that might have been done to the mailbox */
  
  mu_monitor_wrlock (mailbox->monitor);
  /* Before closing we need to remove all the messages
     - to reclaim the memory
     - to prepare for another scan.  */
  for (i = 0; i < mud->umessages_count; i++)
    {
      mbox_message_t mum = mud->umessages[i];
      /* Destroy attached messages.  */
      if (mum)
	{
	  mu_message_destroy (&(mum->message), mum);
	  free (mum);
	}
    }
  if (mud->umessages)
    free (mud->umessages);
  mud->umessages = NULL;
  mud->messages_count = mud->umessages_count = 0;
  mud->size = 0;
  mud->uidvalidity = 0;
  mud->uidnext = 0;
  mu_monitor_unlock (mailbox->monitor);

  return mu_stream_close (mailbox->stream);
}

static int
mbox_remove (mu_mailbox_t mailbox)
{
  mbox_data_t mud = mailbox->data;

  mu_debug (MU_DEBCAT_MAILBOX, MU_DEBUG_TRACE1,
	    ("mbox_remove (%s)", mud->name));
  return unlink (mud->name) == 0 ? 0 : errno;
}

/* Cover function that calls the real thing, mbox_scan(), with
   notification set.  */
static int
mbox_scan (mu_mailbox_t mailbox, size_t msgno, size_t *pcount)
{
  size_t i;
  mbox_data_t mud = mailbox->data;
  mu_debug (MU_DEBCAT_MAILBOX, MU_DEBUG_TRACE1, ("mbox_scan (%s)", mud->name));
  if (! mbox_is_updated (mailbox))
    return mbox_scan0 (mailbox, msgno, pcount, 1);
  /* Since the mailbox is already updated fake the scan. */
  if (msgno > 0)
    msgno--; /* The fist message is number "1", decrement for the C array.  */
  for (i = msgno; i < mud->messages_count; i++)
    {
      size_t tmp = i;
      if (mu_observable_notify (mailbox->observable, MU_EVT_MESSAGE_ADD,
				&tmp) != 0)
	break;
      /* FIXME: Hardcoded value! Must be configurable */
      if (((i + 1) % 50) == 0)
	mu_observable_notify (mailbox->observable, MU_EVT_MAILBOX_PROGRESS,
			      NULL);
    }
  *pcount = mud->messages_count;
  return 0;
}

/* Alain: How to handle a shrink ? meaning, the &^$^@%#@^& user start two
   browsers and deleted emails in one session.  My views is that we should
   scream bloody murder and hunt them with a machette. But for now just play
   dumb, but maybe the best approach is to pack our things and leave
   .i.e exit()/abort(). 

   Sergey: Nope, we shouldn't abort. Handling it with MU_EVT_MAILBOX_CORRUPT
   is sensible enough. The caller must decide what's the best approach
   in this case. The simplest one is reopening the mailbox. Imap4d currently
   does that.

   And yet another thought: if a user reads his mail with a ... browser (!),
   he is incurable anyway. */

static int
mbox_is_updated (mu_mailbox_t mailbox)
{
  mu_off_t size = 0;
  mbox_data_t mud = mailbox->data;
  if (mu_stream_size (mailbox->stream, &size) != 0)
    return 1;
  if (size < mud->size)
    {
      mu_observable_notify (mailbox->observable, MU_EVT_MAILBOX_CORRUPT,
			    mailbox);
      /* And be verbose.  ? */
      mu_diag_output (MU_DIAG_EMERG, _("mailbox corrupted, shrank in size"));
      /* FIXME: should I crash? */
      return 0;
    }
  return (mud->size == size);
}

static int
mbox_expunge (mu_mailbox_t mailbox)
{
  return mbox_expunge0 (mailbox, 1);
}

static int
mbox_sync (mu_mailbox_t mailbox)
{
  return mbox_expunge0 (mailbox, 0);
}

static int
mbox_message_uid (mu_message_t msg, size_t *puid)
{
  mbox_message_t mum = mu_message_get_owner (msg);
  if (puid)
    *puid = mum->uid;
  return 0;
}

static int
mbox_message_qid (mu_message_t msg, mu_message_qid_t *pqid)
{
  mbox_message_t mum = mu_message_get_owner (msg);
  return mu_asprintf (pqid, "%lu", (unsigned long) mum->envel_from);
}


/* Mbox message headers */

#ifdef MU_MBOX_SET_HEADER
static int
mbox_header_fill (void *data, char **pbuf, size_t *plen)
{
  /* FIXME: This would be almost exact copy of _header_fill from
     mailbox/message.c. So there's perhaps no use defining this
     method. Instead we prefer to use the default header machinery,
     which relies on the message stream (see #else below). */
  abort ();
}

static int
_msg_header_setup (mu_message_t msg, mbox_message_t mum)
{
  mu_header_t header;
  int status = mu_header_create (&header, NULL, 0);
  if (status == 0)
    {
      mu_header_set_fill (header, mbox_header_fill, msg);
      mu_message_set_header (msg, header, mum);
    }
  return status;
}
#else
static int
_msg_stream_setup (mu_message_t msg, mbox_message_t mum)
{
  mu_stream_t stream;
  int status;
  
  status = mu_streamref_create_abridged (&stream,
					 mum->mud->mailbox->stream,
					 mum->envel_from_end,
					 mum->body_end - 1);
  if (status == 0)
    status = mu_message_set_stream (msg, stream, mum);
  return status;
}
#endif

/* Mbox message body */
static int
mbox_body_size (mu_body_t body, size_t *psize)
{
  mu_message_t msg = mu_body_get_owner (body);
  mbox_message_t mum = mu_message_get_owner (msg);
  if (mum == NULL)
    return EINVAL;
  if (psize)
    *psize = mum->body_end - mum->body;
  return 0;
}

static int
mbox_body_lines (mu_body_t body, size_t *plines)
{
  mu_message_t msg = mu_body_get_owner (body);
  mbox_message_t mum = mu_message_get_owner (msg);
  if (mum == NULL)
    return EINVAL;
  if (plines)
    *plines = mum->body_lines;
  return 0;
}

static int
_msg_body_setup (mu_message_t msg, mbox_message_t mum)
{
  mu_body_t body;
  mu_stream_t stream;
  int status;
  
  status = mu_body_create (&body, msg);
  if (status)
    return status;
  status = mu_streamref_create_abridged (&stream,
					 mum->mud->mailbox->stream,
					 mum->body,
					 mum->body_end - 1);
  if (status)
    mu_body_destroy (&body, msg);
  else
    {
      mu_body_set_stream (body, stream, msg);
      mu_body_set_size (body, mbox_body_size, msg);
      mu_body_set_lines (body, mbox_body_lines, msg);
      mu_body_clear_modified (body);
      mu_message_set_body (msg, body, mum);
    }
  return status;
}

/* Mbox message envelope */

static int
mbox_envelope_date (mu_envelope_t envelope, char *buf, size_t len,
		    size_t *pnwrite)
{
  mu_message_t msg = mu_envelope_get_owner (envelope);
  mbox_message_t mum = mu_message_get_owner (msg);
  size_t n = 0;
  int status;
  char buffer[512];
  char *s;

  if (mum == NULL)
    return EINVAL;

  status = mu_stream_seek (mum->mud->mailbox->stream,
			   mum->envel_from, MU_SEEK_SET,
			   NULL);
  if (status)
    return status;
  status = mu_stream_readline (mum->mud->mailbox->stream,
			       buffer, sizeof(buffer),
			       &n);
  if (status)
    return status;
  mu_rtrim_class (buffer, MU_CTYPE_ENDLN);
  
  /* Format:  "From [sender] [date]" */
  /* strlen ("From ") == 5 */
  if (n > 5 && (s = strchr (buffer + 5, ' ')) != NULL)
    {
      if (buf && len > 0)
	{
	  len--; /* Leave space for the null.  */
	  strncpy (buf, s + 1, len)[len] = '\0';
	  len = strlen (buf);
	}
      else
	len = strlen (s + 1);
    }
  else
    len = 0;

  if (pnwrite)
    *pnwrite = len;
  return 0;
}

static int
mbox_envelope_sender (mu_envelope_t envelope, char *buf, size_t len,
		      size_t *pnwrite)
{
  mu_message_t msg = mu_envelope_get_owner (envelope);
  mbox_message_t mum = mu_message_get_owner (msg);
  size_t n = 0;
  int status;
  char buffer[512];
  char *s;

  if (mum == NULL)
    return EINVAL;

  status = mu_stream_seek (mum->mud->mailbox->stream,
			   mum->envel_from, MU_SEEK_SET,
			   NULL);
  if (status)
    return status;
  status = mu_stream_readline (mum->mud->mailbox->stream, buffer,
			       sizeof(buffer), &n);
  if (status)
    return status;

  /* Format:  "From [sender] [date]" */
  /* strlen ("From ") == 5 */
  if (n > 5 && (s = strchr (buffer + 5, ' ')) != NULL)
    {
      /* Put a NULL to isolate the sender string, make a C string.  */
      *s = '\0';
      if (buf && len > 0)
	{
	  len--; /* leave space for the null */
	  strncpy (buf, buffer + 5, len)[len] = '\0';
	  len = strlen (buf);
	}
      else
	len = strlen (buffer + 5);
    }
  else
    len = 0;

  if (pnwrite)
    *pnwrite = len;
  return 0;
}

int
_msg_envelope_setup (mu_message_t msg, mbox_message_t mum)
{
  mu_envelope_t envelope;
  int status = mu_envelope_create (&envelope, msg);
  if (status == 0)
    {
      mu_envelope_set_sender (envelope, mbox_envelope_sender, msg);
      mu_envelope_set_date (envelope, mbox_envelope_date, msg);
      mu_message_set_envelope (msg, envelope, mum);
    }
  return status;
}

/* Mbox message attributes */

static int
mbox_get_attr_flags (mu_attribute_t attr, int *pflags)
{
  mu_message_t msg = mu_attribute_get_owner (attr);
  mbox_message_t mum = mu_message_get_owner (msg);

  if (mum == NULL)
    return EINVAL;
  if (pflags)
    *pflags = mum->attr_flags;
  return 0;
}

static int
mbox_set_attr_flags (mu_attribute_t attr, int flags)
{
  mu_message_t msg = mu_attribute_get_owner (attr);
  mbox_message_t mum = mu_message_get_owner (msg);

  if (mum == NULL)
    return EINVAL;
  mum->attr_flags |= flags;
  return 0;
}

static int
mbox_unset_attr_flags (mu_attribute_t attr, int flags)
{
  mu_message_t msg = mu_attribute_get_owner (attr);
  mbox_message_t mum = mu_message_get_owner (msg);

  if (mum == NULL)
    return EINVAL;
  mum->attr_flags &= ~flags;
  return 0;
}

int
_msg_attribute_setup (mu_message_t msg, mbox_message_t mum)
{
  mu_attribute_t attribute;
  int status = mu_attribute_create (&attribute, msg);
  if (status == 0)
    {
      mu_attribute_set_get_flags (attribute, mbox_get_attr_flags, msg);
      mu_attribute_set_set_flags (attribute, mbox_set_attr_flags, msg);
      mu_attribute_set_unset_flags (attribute, mbox_unset_attr_flags, msg);
      mu_message_set_attribute (msg, attribute, mum);
    }
  return status;
}

/* Initialize a message */
static int
_msg_setup (mu_message_t msg, mbox_message_t mum)
{
  int status;
  
#ifdef MU_MBOX_SET_HEADER
  /* Set the header.  */
  status = _msg_header_setup (msg, mum);
#else
  status = _msg_stream_setup (msg, mum);
#endif
  if (status)
    return status;
  
  status = _msg_attribute_setup (msg, mum);
  if (status)
    return status;

  status = _msg_body_setup (msg, mum);
  if (status)
    return status;

  return _msg_envelope_setup (msg, mum);
}

/* Create new mu_message_t from mbox_message_t */
static int
new_message (mu_mailbox_t mailbox, mbox_message_t mum, mu_message_t *pmsg)
{
  int status;
  mu_message_t msg;

  /* Get an empty message struct.  */
  status = mu_message_create (&msg, mum);
  if (status != 0)
    return status;

  status = _msg_setup (msg, mum);
  if (status)
    {
      mu_message_destroy (&msg, mum);
      return status;
    }

  /* Set the UID.  */
  mu_message_set_uid (msg, mbox_message_uid, mum);
  mu_message_set_qid (msg, mbox_message_qid, mum);
  
  /* Attach the message to the mailbox mbox data.  */
  mum->message = msg;
  mu_message_set_mailbox (msg, mailbox, mum);

  mu_message_clear_modified (msg);

  *pmsg = msg;
  
  return 0;
}  

static int
mbox_get_message (mu_mailbox_t mailbox, size_t msgno, mu_message_t *pmsg)
{
  int status;
  mbox_data_t mud = mailbox->data;
  mbox_message_t mum;

  /* Sanity checks.  */
  if (pmsg == NULL)
    return MU_ERR_OUT_PTR_NULL;
  if (mud == NULL || msgno < 1)
    return EINVAL;

  /* If we did not start a scanning yet do it now.  */
  if (mud->messages_count == 0)
    {
      status = mbox_scan0 (mailbox, 1, NULL, 0);
      if (status != 0)
	return status;
    }

  /* Second sanity: check the message number.  */
  if (!(mud->messages_count > 0
	&& msgno > 0
	&& msgno <= mud->messages_count))
    return MU_ERR_NOENT;

  mum = mud->umessages[msgno - 1];

  /* Check if we already have it.  */
  if (mum->message)
    {
      if (pmsg)
	*pmsg = mum->message;
      return 0;
    }

  mu_debug (MU_DEBCAT_MAILBOX, MU_DEBUG_TRACE1, ("mbox_get_message (%s, %lu)",
	     mud->name, (unsigned long) msgno));

  return new_message (mailbox, mum, pmsg);
}

static int
qid2off (mu_message_qid_t qid, mu_off_t *pret)
{
  mu_off_t ret = 0;
  for (;*qid; qid++)
    {
      if (!('0' <= *qid && *qid <= '9'))
	return 1;
      ret = ret * 10 + *qid - '0';
    }
  *pret = ret;
  return 0;
}
      
static int
mbox_quick_get_message (mu_mailbox_t mailbox, mu_message_qid_t qid,
			mu_message_t *pmsg)
{
  int status;
  mbox_data_t mud = mailbox->data;
  mbox_message_t mum;
  mu_off_t offset;
  
  if (mailbox == NULL || qid2off (qid, &offset)
      || !(mailbox->flags & MU_STREAM_QACCESS))
    return EINVAL;

  if (mud->messages_count == 0)
    {
      status = mbox_scan1 (mailbox, offset, 0);
      if (status != 0)
	return status;
      if (mud->messages_count == 0)
	return MU_ERR_NOENT;
    }

  /* Quick access mode retrieves only one message */
  mum = mud->umessages[0]; 

  /* Check if we already have it and verify if it is the right one. */
  if (mum->message)
    {
      char *vqid;
      status = mu_message_get_qid (mum->message, &vqid);
      if (status)
	return status;
      status = strcmp (qid, vqid);
      free (vqid);
      if (status)
	return MU_ERR_EXISTS;
      if (pmsg)
	*pmsg = mum->message;
      return 0;
    }

  return new_message (mailbox, mum, pmsg);
}

static int
mbox_get_size (mu_mailbox_t mailbox, mu_off_t *psize)
{
  mu_off_t size;
  int status;

  /* Maybe was not open yet ??  */
  status  = mu_stream_size (mailbox->stream, &size);
  if (status != 0)
    return status;
  if (psize)
    *psize = size;
  return 0;
}

static int
mbox_messages_count (mu_mailbox_t mailbox, size_t *pcount)
{
  mbox_data_t mud = mailbox->data;

  if (mud == NULL)
    return EINVAL;

  if (! mbox_is_updated (mailbox))
    return mbox_scan0 (mailbox,  mud->messages_count, pcount, 0);

  if (pcount)
    *pcount = mud->messages_count;

  return 0;
}

/* A "recent" message is the one not marked with MU_ATTRIBUTE_SEEN
   ('O' in the Status header), i.e. a message that is first seen
   by the current session (see attributes.h) */
static int
mbox_messages_recent (mu_mailbox_t mailbox, size_t *pcount)
{
  mbox_data_t mud = mailbox->data;
  mbox_message_t mum;
  size_t j, recent;

  /* If we did not start a scanning yet do it now.  */
  if (mud->messages_count == 0)
    {
      int status = mbox_scan0 (mailbox, 1, NULL, 0);
      if (status != 0)
	return status;
    }
  for (recent = j = 0; j < mud->messages_count; j++)
    {
      mum = mud->umessages[j];
      if (mum && MU_ATTRIBUTE_IS_UNSEEN(mum->attr_flags))
	recent++;
    }
  *pcount = recent;
  return 0;
}

/* An "unseen" message is the one that has not been read yet */
static int
mbox_message_unseen (mu_mailbox_t mailbox, size_t *pmsgno)
{
  mbox_data_t mud = mailbox->data;
  mbox_message_t mum;
  size_t j, unseen;

  /* If we did not start a scanning yet do it now.  */
  if (mud->messages_count == 0)
    {
      int status = mbox_scan0 (mailbox, 1, NULL, 0);
      if (status != 0)
	return status;
    }
  for (unseen = j = 0; j < mud->messages_count; j++)
    {
      mum = mud->umessages[j];
      if (mum && MU_ATTRIBUTE_IS_UNREAD(mum->attr_flags))
	{
	  unseen = j + 1;
	  break;
	}
    }
  *pmsgno = unseen;
  return 0;
}

static int
mbox_uidvalidity (mu_mailbox_t mailbox, unsigned long *puidvalidity)
{
  mbox_data_t mud = mailbox->data;
  int status = mbox_messages_count (mailbox, NULL);
  if (status != 0)
    return status;
  /* If we did not start a scanning yet do it now.  */
  if (mud->messages_count == 0)
    {
      status = mbox_scan0 (mailbox, 1, NULL, 0);
      if (status != 0)
	return status;
    }
  if (puidvalidity)
    *puidvalidity = mud->uidvalidity;
  return 0;
}

static int
mbox_uidnext (mu_mailbox_t mailbox, size_t *puidnext)
{
  mbox_data_t mud = mailbox->data;
  int status = mbox_messages_count (mailbox, NULL);
  if (status != 0)
    return status;
  /* If we did not start a scanning yet do it now.  */
  if (mud->messages_count == 0)
    {
      status = mbox_scan0 (mailbox, 1, NULL, 0);
      if (status != 0)
	return status;
    }
  if (puidnext)
    *puidnext = mud->uidnext;
  return 0;
}

#ifdef WITH_PTHREAD
void
mbox_cleanup (void *arg)
{
  mu_mailbox_t mailbox = arg;
  mu_monitor_unlock (mailbox->monitor);
  mu_locker_unlock (mailbox->locker);
}
#endif


/* Support functions for appending a message to a stream or mbox */

#define MBOX_FIRSTMSG 0x01
#define MBOX_EXPUNGE  0x02

int
restore_sender (mu_message_t msg, char **pret)
{
  mu_header_t hdr;
  const char *s = NULL;
  char *from = NULL;
  
  if (mu_message_get_header (msg, &hdr) == 0)
    mu_header_sget_value (hdr, MU_HEADER_FROM, &s);

  if (s)
    {
      int status;
      mu_address_t addr;
      
      status = mu_address_create (&addr, s);
      if (status == 0)
	mu_address_aget_email (addr, 1, &from);
      mu_address_destroy (&addr);
    }

  if (!from)
    {
      from = strdup (PACKAGE);
      if (!from) 
	return ENOMEM;
    }
  *pret = from;
  return 0;
}

int
restore_date (mu_message_t msg, char **pret)
{
  mu_header_t hdr;
  const char *s;
  char *date = NULL;
  struct tm tm;
  struct mu_timezone tz;
  
  if (mu_message_get_header (msg, &hdr) == 0)
    mu_header_sget_value (hdr, MU_HEADER_DATE, &s);

  if (s && mu_scan_datetime (s, MU_DATETIME_SCAN_RFC822, &tm, &tz, NULL) == 0)
    {
      char datebuf[MU_DATETIME_FROM_LENGTH+1];
      
      /* FIXME: Compensate for TZ differences. */
      mu_strftime (datebuf, sizeof datebuf, MU_DATETIME_FROM, &tm);
      date = strdup (datebuf);
    }
  else
    {
      time_t t;
      time (&t);
      date = strdup (ctime (&t));
    }
  if (!date)
    return ENOMEM;
  *pret = date;
  return 0;
}

static int
write_array (mu_stream_t stream, int count, const char **array)
{
  int i;
  for (i = 0; i < count; i++)
    {
      int status = mu_stream_write (stream, array[i], strlen (array[i]), NULL);
      if (status)
	return status;
    }
  return 0;
}

static int
msg_envelope_to_stream (mu_stream_t out, mu_message_t msg)
{
  mu_envelope_t env;
  int status;
  char *sender = NULL;
  char *datestr = NULL;
  const char *envarr[5];
  
  status = mu_message_get_envelope (msg, &env);
  if (status)
    return status;
  
  status = mu_envelope_aget_sender (env, &sender);
  switch (status)
    {
    case 0:
      break;

    case MU_ERR_NOENT:
      status = restore_sender (msg, &sender);
      if (status == 0)
	break;
    default:
      return status;
    }

  status = mu_envelope_aget_date (env, &datestr);
  switch (status)
    {
    case 0:
      break;

    case MU_ERR_NOENT:
      status = restore_date (msg, &datestr);
      if (status == 0)
	break;
      free (sender);
    default:
      return status;
    }

  mu_rtrim_class (datestr, MU_CTYPE_ENDLN);

  envarr[0] = "From ";
  envarr[1] = sender;
  envarr[2] = " ";
  envarr[3] = datestr;
  envarr[4] = "\n";

  status = write_array (out, 5, envarr);
  free (sender);
  free (datestr);
  return status;
}

/* Headers */
static int
msg_header_to_stream (mu_stream_t ostr, mu_message_t msg)
{
  int status;
  mu_header_t header;
  mu_iterator_t itr;
  const char *harr[4];

  status = mu_message_get_header (msg, &header);
  if (status)
    return status;
  status = mu_header_get_iterator (header, &itr);
  if (status)
    return status;

  harr[1] = ": ";
  harr[3] = "\n";
  for (mu_iterator_first (itr);
       status == 0 && !mu_iterator_is_done (itr); mu_iterator_next (itr))
    {
      mu_iterator_current_kv (itr, (const void **)&harr[0], (void **)&harr[2]);
      if (mu_c_strcasecmp (harr[0], MU_HEADER_STATUS) == 0
	  || mu_c_strcasecmp (harr[0], MU_HEADER_X_IMAPBASE) == 0
	  || mu_c_strcasecmp (harr[0], MU_HEADER_X_UID) == 0)
	continue;
      status = write_array (ostr, 4, harr);
    }
  mu_iterator_destroy (&itr);
  return status;
  
}

/* Attributes */
static int
msg_attr_to_stream (mu_stream_t ostr, mu_message_t msg)
{
  mu_attribute_t attr;
  char abuf[MU_STATUS_BUF_SIZE + 1];
  const char *harr[4];
  int status;
  
  status = mu_message_get_attribute (msg, &attr);
  if (status)
    return status;
  status = mu_attribute_to_string (attr, abuf, sizeof (abuf), NULL);
  if (status)
    return status;

  harr[0] = MU_HEADER_STATUS;
  harr[1] = ": ";
  harr[2] = abuf;
  harr[3] = "\n";
  return write_array (ostr, 4, harr);
}

/* X-UID header */

static int
uid_to_stream (mu_stream_t ostr, mu_message_t msg, mbox_data_t mud, int flags)
{
  size_t uid = 0;
  
  if (flags & MBOX_EXPUNGE)
    {
      int status = mu_message_get_uid (msg, &uid);
      if (status)
	return status;
    }
  else
    uid = mud->uidnext++;
  
  mu_stream_printf (ostr, "%s: %u\n", MU_HEADER_X_UID, (unsigned) uid);
  if (mu_stream_err (ostr))
    return mu_stream_last_error (ostr);
  return 0;
}

/* Append MSG to stream OSTR in its current position */
static int
append_message_to_stream (mu_stream_t ostr, mu_message_t msg,
			  mbox_data_t mud, int flags)
{
  int status;
  mu_stream_t istr, flt;
  
  status = msg_envelope_to_stream (ostr, msg);
  if (status)
    return status;

  if (flags & MBOX_EXPUNGE)
    {
      mu_body_t body;

      status = msg_header_to_stream (ostr, msg);
      if (status)
	return status;

      if (flags & MBOX_FIRSTMSG)
	{
	  /* FIXME: Perhaps printf should return error code,
	     like the rest of stream functions. */
	  mu_stream_printf (ostr, "%s: %lu %u\n",
			    MU_HEADER_X_IMAPBASE,
			    (unsigned long) mud->uidvalidity,
			    (unsigned) mud->uidnext);
	  if (mu_stream_err (ostr))
	    return mu_stream_last_error (ostr);
	}

      status = msg_attr_to_stream (ostr, msg);
      if (status)
	return status;

      status = uid_to_stream (ostr, msg, mud, flags);
      if (status)
	return status;

      status = mu_stream_write (ostr, "\n", 1, NULL);
      if (status)
	return status;
      
      status = mu_message_get_body (msg, &body);
      if (status)
	return status;
      status = mu_body_get_streamref (body, &istr);
      if (status)
	return status;
    }
  else
    {
      status = mu_message_get_streamref (msg, &istr);
      if (status)
	return status;
    }

  status = mu_filter_create (&flt, istr, "FROM",
			     MU_FILTER_ENCODE, MU_STREAM_READ);
  mu_stream_unref (istr);
  if (status == 0)
    {
      status = mu_stream_copy (ostr, flt, 0, NULL);
      mu_stream_destroy (&flt);
      if (status == 0)
	status = mu_stream_write (ostr, "\n", 1, NULL);
    }
  
  return status;
}

static int
mbox_append_message (mu_mailbox_t mailbox, mu_message_t msg)
{
  int status = 0;
  mbox_data_t mud = mailbox->data;
  mu_off_t size;
  
  if (msg == NULL || mud == NULL)
    return EINVAL;

  mu_debug (MU_DEBCAT_MAILBOX, MU_DEBUG_TRACE1, ("mbox_append_message (%s)",
	     mud->name));

  if (mailbox->locker &&
      (status = mu_locker_lock (mailbox->locker)) != 0)
    {
      mu_debug (MU_DEBCAT_MAILBOX, MU_DEBUG_TRACE1,
		("mbox_append_message: %s", mu_strerror(status)));
      return status;
    }

  status = mu_stream_seek (mailbox->stream, 0, MU_SEEK_END, &size);
  if (status)
    return status;
  status = append_message_to_stream (mailbox->stream, msg, mud, 0);
  if (mailbox->locker)
    mu_locker_unlock (mailbox->locker);

  if (status)
    {
      int rc = mu_stream_truncate (mailbox->stream, size);
      if (rc)
	mu_error (_("cannot truncate stream after failed append: %s"),
		  mu_stream_strerror (mailbox->stream, rc));
      return status;
    }
  
  if (mailbox->observable)
    {
      char *buf = NULL;
      mu_asprintf (&buf, "%lu", (unsigned long) size);
      mu_observable_notify (mailbox->observable, 
                            MU_EVT_MAILBOX_MESSAGE_APPEND, buf);
      free (buf);
    }
  
  return 0;
}


static void
mbox_reset (mu_mailbox_t mailbox)
{
  mbox_data_t mud = mailbox->data;
  size_t i;

  mu_monitor_wrlock (mailbox->monitor);
  for (i = 0; i < mud->messages_count; i++)
    {
      /* Clear all the references */
      mbox_message_t mum = mud->umessages[i];
      mu_message_destroy (&mum->message, mum);
      memset (mum, 0, sizeof (*mum));
      mum->envel_from = mum->envel_from_end = 0;
      mum->body = mum->body_end = 0;
      mum->header_lines = mum->body_lines = 0;
    }
  mu_monitor_unlock (mailbox->monitor);
  /* This resets the messages_count, the last argument 0 means
     not to send event notification.  */
  mbox_scan0 (mailbox, 1, NULL, 0);
}

static int
mbox_expunge_unlocked (mu_mailbox_t mailbox, size_t dirty, int remove_deleted,
		       mu_stream_t tempstr)
{
  mbox_data_t mud = mailbox->data;
  int status;
  size_t i;
  size_t save_imapbase = 0;  /* uidvalidity is save in the first message.  */
  mu_off_t start_off;
  mu_off_t size;
  size_t expcount = 0;
  
  /* Set the marker position.  */
  start_off = mud->umessages[dirty]->envel_from;

  for (i = dirty; i < mud->messages_count; i++)
    {
      mbox_message_t mum = mud->umessages[i];
      
      if (remove_deleted && ATTRIBUTE_IS_DELETED (mum->attr_flags))
	{
	  size_t expevt[2] = { i + 1, expcount };
	  mu_observable_notify (mailbox->observable,
				MU_EVT_MAILBOX_MESSAGE_EXPUNGE,
				expevt);
	  expcount++;
	  mu_message_destroy (&mum->message, mum);
	  /* We save the uidvalidity in the first message, if it is being
	     deleted we need to move the uidvalidity to the first available
	     (non-deleted) message.  */
	  if (i == save_imapbase)
	    {
	      save_imapbase = i + 1;
	      if (save_imapbase < mud->messages_count)
		mud->umessages[save_imapbase]->attr_flags
		  |= MU_ATTRIBUTE_MODIFIED;
	    }
	  continue;
	}

      if ((mum->attr_flags & MU_ATTRIBUTE_MODIFIED) ||
	  (mum->message && mu_message_is_modified (mum->message)))
	{
	  /* Use relatively expensive append_message_to_stream only if
	     the message is marked dirty.  */
	  
	  int flags = MBOX_EXPUNGE |
	                ((i == save_imapbase) ? MBOX_FIRSTMSG : 0);
					     
	  /* The message was not instantiated, probably the dirty flag was
	     set by mbox_scan(), create one here.  */
	  if (mum->message == 0)
	    {
	      mu_message_t msg;
	      status = mbox_get_message (mailbox, i + 1, &msg);
	      if (status != 0)
		{
		  mu_error (_("%s:%d: error expunging: %s"),	        
			    __FILE__, __LINE__, mu_strerror (status));	
		  return status;
		}
	    }
	  status = append_message_to_stream (tempstr, mum->message, mud,
					     flags);
	  if (status != 0)
	    {
	      mu_error (_("%s:%d: error expunging: %s"),	        
			__FILE__, __LINE__, mu_strerror (status));	
	      return status;
	    }
	  /* Clear the dirty bits.  */
	  mum->attr_flags &= ~MU_ATTRIBUTE_MODIFIED;
	  mu_message_clear_modified (mum->message);
	}
      else
	{
	  /* Otherwise, copy bits from mailbox->stream as is. */
	  
	  status = mu_stream_seek (mailbox->stream, mum->envel_from,
				   MU_SEEK_SET, NULL);
	  if (status)
	    {
	      mu_error (_("%s:%d: seek error: %s"),
			__FILE__, __LINE__,
		        mu_stream_strerror (mailbox->stream, status));
	      return status;
	    }
	  status = mu_stream_copy (tempstr, mailbox->stream,
				   mum->body_end - mum->envel_from, NULL);
	  if (status)
	    {
	      mu_error (_("%s:%d: error copying: %s"),
			__FILE__, __LINE__,
		        mu_strerror (status));
	      return status;
	    }
	}
    }

  /* Caution: before moving data back to the mailbox see
     - if we've receive new mails.  Some programs may not respect the lock,
     - or the lock was held for too long.
     - The mailbox may not have been properly updated before expunging.  */
  if (mu_stream_size (mailbox->stream, &size) == 0)
    {
      if (size > mud->size)
	{
	  mu_off_t len = size - mud->size;

	  status = mu_stream_seek (mailbox->stream, mud->size, MU_SEEK_SET,
				   NULL);
	  if (status)
	    {
	      mu_error (_("%s:%d: seek error: %s"),
			__FILE__, __LINE__,
			mu_stream_strerror (mailbox->stream, status));
	      return status;
	    }
  
	  status = mu_stream_copy (tempstr, mailbox->stream, len, NULL);
	  if (status)
	    {
	      mu_error (_("%s:%d: error writing to temporary stream: %s"),
			__FILE__, __LINE__,
			mu_strerror (status));
	      return status;
	    }
	}
      else if (size < mud->size)
	{
	  /* Corrupted mailbox. */
	  mu_observable_notify (mailbox->observable, MU_EVT_MAILBOX_CORRUPT,
				mailbox);
	  mu_error (_("%s:%d: mailbox shrunk while expunging"),
		    __FILE__, __LINE__);
	  return MU_ERR_FAILURE; /* FIXME: need an error code for that */
	}
    }

  
  /* Copy data from tempstr back to the mailbox. */
  status = mu_stream_seek (mailbox->stream, start_off, MU_SEEK_SET, NULL);
  if (status)
    {
      mu_error (_("%s:%d: seek error: %s"),
		__FILE__, __LINE__,
		mu_stream_strerror (mailbox->stream, status));
      return status;
    }
  
  status = mu_stream_size (tempstr, &size);
  if (status)
    {
      mu_error (_("%s:%d: cannot get size of the temp stream: %s"),
		__FILE__, __LINE__,
		mu_stream_strerror (tempstr, status));
      return status;
    }
  
  status = mu_stream_seek (tempstr, 0, MU_SEEK_SET, NULL);
  if (status)
    {
      mu_error (_("%s:%d: seek error: %s"),
		__FILE__, __LINE__,
		mu_stream_strerror (mailbox->stream, status));
      return status;
    }

  status = mu_stream_copy (mailbox->stream, tempstr, size, NULL);
  if (status)
    {
      mu_error (_("%s:%d: copying from the temporary stream: %s"),
		__FILE__, __LINE__,
		mu_strerror (status));
      return status;
    }
  
  status = mu_stream_truncate (mailbox->stream, start_off + size);
  if (status)
    {
      mu_error (_("%s:%d: error truncating stream: %s"),
		__FILE__, __LINE__,
		mu_strerror (status));
    }
  return status;
}

static int
mbox_expunge0 (mu_mailbox_t mailbox, int remove_deleted)
{
  mbox_data_t mud = mailbox->data;
  size_t dirty;
  int status;
  mu_stream_t tempstr;
  
  if (mud == NULL)
    return EINVAL;

  mu_debug (MU_DEBCAT_MAILBOX, MU_DEBUG_TRACE1,
	    ("mbox_expunge (%s)", mud->name));

  /* Noop.  */
  if (mud->messages_count == 0)
    return 0;

  /* Find first dirty (modified) message. */
  for (dirty = 0; dirty < mud->messages_count; dirty++)
    {
      mbox_message_t mum = mud->umessages[dirty];
      /* Message may have been tampered, break here.  */
      if ((mum->attr_flags & MU_ATTRIBUTE_MODIFIED) ||
	  (mum->attr_flags & MU_ATTRIBUTE_DELETED) ||
	  (mum->message && mu_message_is_modified (mum->message)))
	break;
    }

  /* Did something change ?  */
  if (dirty == mud->messages_count)
    return 0; /* Nothing changed.  */

  /* Lock the mailbox */
  if (mailbox->locker &&
      (status = mu_locker_lock (mailbox->locker)) != 0)
    return status;

  status = mu_temp_file_stream_create (&tempstr, NULL, 0);
  if (status == 0)
    {
      sigset_t signalset;
#ifdef WITH_PTHREAD
      int state;
      pthread_setcancelstate (PTHREAD_CANCEL_DISABLE, &state);
#endif
      sigemptyset (&signalset);
      sigaddset (&signalset, SIGTERM);
      sigaddset (&signalset, SIGHUP);
      sigaddset (&signalset, SIGTSTP);
      sigaddset (&signalset, SIGINT);
      sigaddset (&signalset, SIGWINCH);
      sigprocmask (SIG_BLOCK, &signalset, 0);
      
      status = mbox_expunge_unlocked (mailbox, dirty, remove_deleted,
				      tempstr);

#ifdef WITH_PTHREAD
      pthread_setcancelstate (state, &state);
#endif
      sigprocmask (SIG_UNBLOCK, &signalset, 0);
      
      mu_stream_destroy (&tempstr);
      
      if (status == 0)
	mbox_reset (mailbox);
    }
  if (mailbox->locker)
    mu_locker_unlock (mailbox->locker);
  return status;
}


/* Allocate the mbox_data_t struct(concrete mailbox), but don't do any
   parsing on the name or even test for existence.  However we do strip any
   leading "mbox:" part of the name, this is suppose to be the
   protocol/scheme name.  */
int
_mailbox_mbox_init (mu_mailbox_t mailbox)
{
  int status;
  mbox_data_t mud;

  if (mailbox == NULL)
    return EINVAL;

  /* Allocate specific mbox data.  */
  mud = mailbox->data = calloc (1, sizeof (*mud));
  if (mailbox->data == NULL)
    return ENOMEM;

  /* Back pointer.  */
  mud->mailbox = mailbox;

  /* Copy the name:
     We do not do any further interpretation after the scheme "mbox:"
     Because for example on distributed system like QnX4 a file name is
     //390/etc/passwd.  So the best approach is to let the OS handle it
     for example if we receive: "mbox:///var/mail/alain" the mailbox name
     will be "///var/mail/alain", we let open() do the right thing.
     So it will let things like this "mbox://390/var/mail/alain" where
     the "//" _is_ part of the filename, pass correctely.  */
  status = mu_url_aget_path (mailbox->url, &mud->name);
  if (status)
    {
      free (mud);
      mailbox->data = NULL;
      return status;
    }

  /* Overloading the defaults.  */
  mailbox->_destroy = mbox_destroy;

  mailbox->_open = mbox_open;
  mailbox->_close = mbox_close;
  mailbox->_remove = mbox_remove;
  
  /* Overloading of the entire mailbox object methods.  */
  mailbox->_get_message = mbox_get_message;
  mailbox->_append_message = mbox_append_message;
  mailbox->_messages_count = mbox_messages_count;
  mailbox->_messages_recent = mbox_messages_recent;
  mailbox->_message_unseen = mbox_message_unseen;
  mailbox->_expunge = mbox_expunge;
  mailbox->_sync = mbox_sync;
  mailbox->_uidvalidity = mbox_uidvalidity;
  mailbox->_uidnext = mbox_uidnext;
  mailbox->_quick_get_message = mbox_quick_get_message;

  mailbox->_scan = mbox_scan;
  mailbox->_is_updated = mbox_is_updated;

  mailbox->_get_size = mbox_get_size;

  /* Set our properties.  */
  {
    mu_property_t property = NULL;
    mu_mailbox_get_property (mailbox, &property);
    mu_property_set_value (property, "TYPE", "MBOX", 1);
  }

  mu_debug (MU_DEBCAT_MAILBOX, MU_DEBUG_TRACE1, ("mbox_init (%s)", mud->name));
  return 0; /* okdoke */
}

