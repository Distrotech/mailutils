/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Library Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with GNU Mailutils; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* First draft by Sergey Poznyakoff */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#ifdef ENABLE_MH

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <dirent.h>

#ifdef WITH_PTHREAD
# ifdef HAVE_PTHREAD_H
#  define _XOPEN_SOURCE  500
#  include <pthread.h>
# endif
#endif

#include <string.h>
#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif

#include <mailutils/attribute.h>
#include <mailutils/body.h>
#include <mailutils/debug.h>
#include <mailutils/envelope.h>
#include <mailutils/error.h>
#include <mailutils/header.h>
#include <mailutils/locker.h>
#include <mailutils/message.h>
#include <mailutils/mutil.h>
#include <mailutils/property.h>
#include <mailutils/stream.h>
#include <mailutils/url.h>
#include <mailutils/observer.h>
#include <mailbox0.h>
#include <registrar0.h>

#define MAX_OPEN_STREAMS 16

#define MH_ENV_SENDER_HEADER "X-Envelope-Sender"
#define MH_ENV_DATE_HEADER "X-Envelope-Date"

/* Notifications ADD_MESG. */
#define DISPATCH_ADD_MSG(mbox,mhd) \
do \
{ \
  int bailing = 0; \
  monitor_unlock (mbox->monitor); \
  if (mbox->observable) \
     bailing = observable_notify (mbox->observable, MU_EVT_MESSAGE_ADD); \
  if (bailing != 0) \
    { \
      if (pcount) \
        *pcount = (mhd)->msg_count; \
      locker_unlock (mbox->locker); \
      return EINTR; \
    } \
  monitor_wrlock (mbox->monitor); \
} while (0);

/* Note: In this particular implementation the message sequence number
   serves also as its UID. This allows to avoid many problems related
   to keeping the uids in the headers of the messages. */

struct _mh_data;
struct _mh_message
{
  struct _mh_message *next;
  struct _mh_message *prev;

  stream_t stream;          /* Associated file stream */
  off_t body_start;         /* Offset of body start in the message file */
  off_t body_end;           /* Offset of body end (size of file, effectively)*/

  size_t seq_number;        /* message sequence number */

  int attr_flags;           /* Attribute flags */
  int deleted;              /* Was the message originally deleted */

  time_t mtime;             /* Time of last modification */
  size_t header_lines;      /* Number of lines in the header part */
  size_t body_lines;        /* Number of lines in the body */

  message_t message; /* Corresponding message_t */
  struct _mh_data *mhd; /* Back pointer.  */
};

struct _mh_data
{
  /* List of messages: */
  size_t msg_count; /* number of messages in the list */
  struct _mh_message *msg_head;  /* First */
  struct _mh_message *msg_tail;  /* Last */

  unsigned long uidvalidity;

  char *name;                    /* Directory name */

  /* Pool of open message streams */
  struct _mh_message *msg_pool[MAX_OPEN_STREAMS];
  int pool_first;    /* Index to the first used entry in msg_pool */
  int pool_last;     /* Index to the first free entry in msg_pool */

  time_t mtime;      /* Time of last modification */

  mailbox_t mailbox; /* Back pointer. */
};

static void mh_destroy __P((mailbox_t mailbox));
static int mh_open __P ((mailbox_t, int));
static int mh_close __P ((mailbox_t));
static int mh_get_message __P ((mailbox_t, size_t, message_t *));
static int mh_append_message __P ((mailbox_t, message_t));
static int mh_messages_count __P ((mailbox_t, size_t *));
static int mh_messages_recent __P ((mailbox_t, size_t *));
static int mh_message_unseen __P ((mailbox_t, size_t *));
static int mh_expunge __P ((mailbox_t));
static int mh_save_attributes __P ((mailbox_t));
static int mh_uidvalidity __P ((mailbox_t, unsigned long *));
static int mh_uidnext __P ((mailbox_t, size_t *));
static int mh_scan __P ((mailbox_t, size_t, size_t *));
static int mh_scan0 __P ((mailbox_t mailbox, size_t msgno, size_t *pcount,
			  int do_notify));
static int mh_is_updated __P ((mailbox_t));
static int mh_get_size __P ((mailbox_t, off_t *));

static int mh_body_read __P ((stream_t, char *, size_t, off_t, size_t *));
static int mh_body_readline __P ((stream_t, char *, size_t, off_t, size_t *));
static int mh_stream_size __P ((stream_t stream, off_t *psize));

static int mh_body_size __P ((body_t body, size_t *psize));
static int mh_body_lines __P ((body_t body, size_t *plines));

static int mh_message_uid __P ((message_t msg, size_t *puid));

static int mh_message_stream_open __P((struct _mh_message *mhm));
static void mh_message_stream_close __P((struct _mh_message *mhm));

static int mh_header_fill __P((header_t header, char *buffer, size_t len,
			       off_t off, size_t *pnread));
static int mh_header_size __P((header_t header, size_t *psize));
static int mh_header_lines __P((header_t header, size_t *plines));

static int mh_get_attr_flags __P((attribute_t attr, int *pflags));
static int mh_set_attr_flags __P((attribute_t attr, int flags));
static int mh_unset_attr_flags __P((attribute_t attr, int flags));

static void _mh_message_insert __P((struct _mh_data *mhd,
				    struct _mh_message *msg));
static void _mh_message_delete __P((struct _mh_data *mhd,
				    struct _mh_message *msg));
static int mh_pool_open __P((struct _mh_message *mhm));

static int mh_envelope_date __P((envelope_t envelope, char *buf, size_t len,
				 size_t *psize));
static int mh_envelope_sender __P((envelope_t envelope, char *buf, size_t len,
				   size_t *psize));

/* Should be in an other header file.  */
extern int mh_message_number __P ((message_t msg, size_t *pnum));

/* Return filename for the message.
   NOTE: Allocates memory. */
static char *
_mh_message_name (struct _mh_message *mhm, int deleted)
{
  char *filename;
  size_t len = strlen (mhm->mhd->name) + 32;
  filename = malloc (len);
  if (deleted)
    snprintf (filename, len, "%s/,%lu", mhm->mhd->name,
	      (unsigned long) mhm->seq_number);
  else
    snprintf (filename, len, "%s/%lu", mhm->mhd->name,
	      (unsigned long) mhm->seq_number);
  return filename;
}

int
_mailbox_mh_init (mailbox_t mailbox)
{
  struct _mh_data *mhd;
  size_t name_len;

  if (mailbox == NULL)
    return EINVAL;

  mhd = mailbox->data = calloc (1, sizeof (*mhd));
  if (mailbox->data == NULL)
    return ENOMEM;

  /* Back pointer.  */
  mhd->mailbox = mailbox;

  url_get_path (mailbox->url, NULL, 0, &name_len);
  mhd->name = calloc (name_len + 1, sizeof (char));
  if (mhd->name == NULL)
    {
      free (mhd);
      mailbox->data = NULL;
      return ENOMEM;
    }
  url_get_path (mailbox->url, mhd->name, name_len + 1, NULL);

  /* Overloading the defaults.  */
  mailbox->_destroy = mh_destroy;

  mailbox->_open = mh_open;
  mailbox->_close = mh_close;

  /* Overloading of the entire mailbox object methods.  */
  mailbox->_get_message = mh_get_message;
  mailbox->_append_message = mh_append_message;
  mailbox->_messages_count = mh_messages_count;
  mailbox->_messages_recent = mh_messages_recent;
  mailbox->_message_unseen = mh_message_unseen;
  mailbox->_expunge = mh_expunge;
  mailbox->_save_attributes = mh_save_attributes;
  mailbox->_uidvalidity = mh_uidvalidity;
  mailbox->_uidnext = mh_uidnext;

  mailbox->_scan = mh_scan;
  mailbox->_is_updated = mh_is_updated;

  mailbox->_get_size = mh_get_size;

  /* Set our properties.  */
  {
    property_t property = NULL;
    mailbox_get_property (mailbox, &property);
    property_set_value (property, "TYPE", "MH", 1);
  }

  MAILBOX_DEBUG1 (mailbox, MU_DEBUG_TRACE, "mh_init(%s)\n", mhd->name);
  return 0;
}

static void
mh_destroy (mailbox_t mailbox)
{
  struct _mh_data *mhd = mailbox->data;
  struct _mh_message *msg, *next;

  if (!mhd)
    return;

  monitor_wrlock (mailbox->monitor);
  msg = mhd->msg_head;
  while (msg)
    {
      next = msg->next;
      message_destroy (&msg->message, msg);
      free (msg);
      msg = next;
    }

  if (mhd->name)
    free (mhd->name);

  free (mhd);
  mailbox->data = NULL;
  monitor_unlock (mailbox->monitor);
}

static int
mh_open (mailbox_t mailbox, int flags)
{
  struct _mh_data *mhd = mailbox->data;
  int status = 0;
  struct stat st;

  mailbox->flags = flags;
  if (stat (mhd->name, &st) < 0)
    return errno;

  if (!S_ISDIR (st.st_mode))
    return EINVAL;

  mhd->mtime = st.st_mtime;

  /* FIXME: is this the right kind of locking for mh folders? */
  if (mailbox->locker == NULL)
    status = locker_create (&mailbox->locker, mhd->name, 0);
  return 0;
}

static int
mh_close (mailbox_t mailbox)
{
  if (!mailbox)
    return EINVAL;
  return locker_unlock (mailbox->locker);
}

static struct _mh_message *
_mh_get_message (struct _mh_data *mhd, size_t msgno)
{
  size_t n;
  struct _mh_message *msg;

  for (n = 1, msg = mhd->msg_head; msg && n < msgno; n++, msg = msg->next)
    ;

  return msg;
}

/* Find the message with the given sequence number */
static struct _mh_message *
_mh_get_message_seq (struct _mh_data *mhd, size_t seq)
{
  struct _mh_message *msg;

  for (msg = mhd->msg_head; msg && msg->seq_number < seq; msg = msg->next)
    ;

  if (msg)
    return msg->seq_number == seq ? msg : NULL;
  return NULL;
}

static int
_mh_attach_message (mailbox_t mailbox, struct _mh_message *mhm,
		    message_t *pmsg)
{
  int status;
  message_t msg;

  /* Check if we already have it.  */
  if (mhm->message)
    {
      if (pmsg)
	*pmsg = mhm->message;
      return 0;
    }

  /* Get an empty message struct.  */
  status = message_create (&msg, mhm);
  if (status != 0)
    return status;

  /* Set the header.  */
  {
    header_t header = NULL;
    status = header_create (&header, NULL, 0, msg);
    if (status != 0)
      {
	message_destroy (&msg, mhm);
	return status;
      }
    header_set_fill (header, mh_header_fill, msg);
    header_set_size (header, mh_header_size, msg);
    header_set_lines (header, mh_header_lines, msg);
    /*FIXME:
    header_set_get_fvalue (header, mh_header_get_fvalue, msg);
    */
    message_set_header (msg, header, mhm);
  }

  /* Set the attribute.  */
  {
    attribute_t attribute;
    status = attribute_create (&attribute, msg);
    if (status != 0)
      {
	message_destroy (&msg, mhm);
	return status;
      }
    attribute_set_get_flags (attribute, mh_get_attr_flags, msg);
    attribute_set_set_flags (attribute, mh_set_attr_flags, msg);
    attribute_set_unset_flags (attribute, mh_unset_attr_flags, msg);
    message_set_attribute (msg, attribute, mhm);
  }

  /* Prepare the body.  */
  {
    body_t body = NULL;
    stream_t stream = NULL;
    if ((status = body_create (&body, msg)) != 0
	|| (status = stream_create (&stream,
				    mailbox->flags | MU_STREAM_SEEKABLE,
				    body)) != 0)
      {
	body_destroy (&body, msg);
	stream_destroy (&stream, body);
	message_destroy (&msg, mhm);
	return status;
      }
    stream_set_read (stream, mh_body_read, body);
    stream_set_readline (stream, mh_body_readline, body);
    stream_set_size (stream, mh_stream_size, body);
    body_set_stream (body, stream, msg);
    body_set_size (body, mh_body_size, msg);
    body_set_lines (body, mh_body_lines, msg);
    message_set_body (msg, body, mhm);
  }

  /* Set the envelope.  */
  {
    envelope_t envelope = NULL;
    status = envelope_create (&envelope, msg);
    if (status != 0)
      {
	message_destroy (&msg, mhm);
	return status;
      }
    envelope_set_sender (envelope, mh_envelope_sender, msg);
    envelope_set_date (envelope, mh_envelope_date, msg);
    message_set_envelope (msg, envelope, mhm);
  }

  /* Set the UID.  */
  message_set_uid (msg, mh_message_uid, mhm);

  /* Attach the message to the mailbox mbox data.  */
  mhm->message = msg;
  message_set_mailbox (msg, mailbox, mhm);

  if (pmsg)
    *pmsg = msg;

  return 0;
}

static int
mh_get_message (mailbox_t mailbox, size_t msgno, message_t *pmsg)
{
  int status;
  struct _mh_data *mhd = mailbox->data;
  struct _mh_message *mhm;

  /* Sanity checks.  */
  if (pmsg == NULL || mhd == NULL)
    return EINVAL;

  /* If we did not start a scanning yet do it now.  */
  if (mhd->msg_count == 0)
    {
      status = mh_scan0 (mailbox, 1, NULL, 0);
      if (status != 0)
	return status;
    }

  if ((mhm = _mh_get_message (mhd, msgno)) == NULL)
    return EINVAL;
  return _mh_attach_message (mailbox, mhm, pmsg);
}

static size_t
_mh_next_seq (struct _mh_data *mhd)
{
  return (mhd->msg_tail ? mhd->msg_tail->seq_number : 0) + 1;
}

static FILE *
_mh_tempfile(struct _mh_data *mhd, char **namep)
{
  int fd = mu_tempfile (mhd->name, namep);
  if (fd == -1)
      return NULL;
  return fdopen (fd, "w");
}

static int
_mh_message_save (struct _mh_data *mhd, struct _mh_message *mhm, int expunge)
{
  stream_t stream = NULL;
  char *name = NULL, *buf = NULL, *msg_name;
  size_t n, off = 0;
  size_t bsize;
  size_t nlines;
  FILE *fp;
  message_t msg = mhm->message;
  header_t hdr;
  int status;
  attribute_t attr;
  body_t body;
  char buffer[512];
  envelope_t env = NULL;
  
  if (expunge)
    fp = _mh_tempfile (mhm->mhd, &name);
  else
    {
      msg_name = _mh_message_name (mhm, mhm->deleted);
      fp = fopen (msg_name, "w");
      free (msg_name);
    }

  if (!fp)
    {
      free (mhm);
      return errno;
    }

  message_size (msg, &bsize);

  /* Try to allocate large buffer */
  for (; bsize > 1; bsize /= 2)
    if ((buf = malloc (bsize)))
      break;

  if (!bsize)
    {
      free (mhm);
      return ENOMEM;
    }

  /* Copy flags */
  message_get_header (msg, &hdr);
  header_get_stream (hdr, &stream);
  off = 0;
  nlines = 0;
  while ((status = stream_readline (stream, buf, bsize, off, &n)) == 0
	 && n != 0)
    {
      if (buf[0] == '\n')
	break;

      nlines++;

      if (!(strncasecmp (buf, "status:", 7) == 0
	    || strncasecmp (buf, "x-imapbase:", 11) == 0
	    || strncasecmp (buf, "x-uid:", 6) == 0
	    || strncasecmp (buf, MH_ENV_DATE_HEADER ":", sizeof (MH_ENV_DATE_HEADER)) == 0
	    || strncasecmp (buf, MH_ENV_SENDER_HEADER ":", sizeof (MH_ENV_SENDER_HEADER)) == 0))
	fprintf (fp, "%s", buf);
      off += n;
    }

  mhm->header_lines = nlines;
  mhm->body_start = off;

  /* Add imapbase */
  if (!mhd->msg_head || (mhd->msg_head == mhm)) /*FIXME*/
    fprintf (fp, "X-IMAPbase: %lu %u\n",
	     (unsigned long) mhd->uidvalidity, (unsigned) _mh_next_seq(mhd));

  message_get_envelope (msg, &env);
  if (envelope_date (env, buffer, sizeof buffer, &n) == 0 && n > 0)
    {
      /* NOTE: buffer is terminated with \n */
      char *p = buffer;
      while (isspace (*p))
	p++;
      fprintf (fp, "%s: %s", MH_ENV_DATE_HEADER, p);
    }
	  
  if (envelope_sender (env, buffer, sizeof buffer, &n) == 0 && n > 0)
    fprintf (fp, "%s: %s\n", MH_ENV_SENDER_HEADER, buffer);
  
  /* Add status */
  message_get_attribute (msg, &attr);
  attribute_to_string (attr, buf, bsize, &n);
  fprintf (fp, "%s", buf);
  fprintf (fp, "\n");
  
  /* Copy message body */

  message_get_body (msg, &body);
  body_get_stream (body, &stream);
  off = 0;
  nlines = 0;
  while (stream_read (stream, buf, bsize, off, &n) == 0 && n != 0)
    {
      char *p;
      for (p = buf; p < buf + n; p++)
	if (*p == '\n')
	  nlines++;
      fwrite (buf, 1, n, fp);
      off += n;
    }

  mhm->body_lines = nlines;
  mhm->body_end = off;

  free (buf);
  fclose (fp);

  if (expunge)
    {
      msg_name = _mh_message_name (mhm, mhm->deleted);
      rename (name, msg_name);
      free (name);
      free (msg_name);
    }

  return 0;
}

static int
mh_append_message (mailbox_t mailbox, message_t msg)
{
  int status;
  struct _mh_data *mhd = mailbox->data;
  struct _mh_message *mhm;

  if (!mailbox || !msg)
    return EINVAL;

  mhm = calloc (1, sizeof(*mhm));
  if (!mhm)
    return ENOMEM;

  /* If we did not start a scanning yet do it now.  */
  if (mhd->msg_count == 0)
    {
      status = mh_scan0 (mailbox, 1, NULL, 0);
      if (status != 0)
	return status;
    }

  mhm->mhd = mhd;
  mhm->seq_number = _mh_next_seq (mhd);
  mhm->message = msg;
  status = _mh_message_save (mhd, mhm, 0);
  mhm->message = NULL;
  /* Insert and re-scan the message */
  _mh_message_insert (mhd, mhm);
  return status;
}

static int
mh_messages_count (mailbox_t mailbox, size_t *pcount)
{
  struct _mh_data *mhd = mailbox->data;

  if (mhd == NULL)
    return EINVAL;

  if (!mh_is_updated (mailbox))
    return mh_scan0 (mailbox,  mhd->msg_count, pcount, 0);

  if (pcount)
    *pcount = mhd->msg_count;

  return 0;
}

/* A "recent" message is the one not marked with MU_ATTRIBUTE_SEEN
   ('O' in the Status header), i.e. a message that is first seen
   by the current session (see attributes.h) */
static int
mh_messages_recent (mailbox_t mailbox, size_t *pcount)
{
  struct _mh_data *mhd = mailbox->data;
  struct _mh_message *mhm;
  size_t count;

  /* If we did not start a scanning yet do it now.  */
  if (mhd->msg_count == 0)
    {
      int status = mh_scan0 (mailbox, 1, NULL, 0);
      if (status != 0)
	return status;
    }
  count = 0;
  for (mhm = mhd->msg_head; mhm; mhm = mhm->next)
    {
      if (MU_ATTRIBUTE_IS_UNSEEN(mhm->attr_flags))
	count++;
    }
  *pcount = count;
  return 0;
}

/* An "unseen" message is the one that has not been read yet */
static int
mh_message_unseen (mailbox_t mailbox, size_t *pmsgno)
{
  struct _mh_data *mhd = mailbox->data;
  struct _mh_message *mhm;
  size_t i, unseen;

  /* If we did not start a scanning yet do it now.  */
  if (mhd->msg_count == 0)
    {
      int status = mh_scan0 (mailbox, 1, NULL, 0);
      if (status != 0)
	return status;
    }
  for (unseen = i = 1, mhm = mhd->msg_head; mhm; i++, mhm = mhm->next)
    {
      if (MU_ATTRIBUTE_IS_UNREAD(mhm->attr_flags))
	{
	  unseen = i;
	  break;
	}
    }
  *pmsgno = unseen;
  return 0;
}

static int
mh_expunge (mailbox_t mailbox)
{
  struct _mh_data *mhd = mailbox->data;
  struct _mh_message *mhm;

  if (mhd == NULL)
    return EINVAL;

  if (mhd->msg_count == 0)
    return 0;

  /* Find the first dirty(modified) message.  */
  for (mhm = mhd->msg_head; mhm; mhm = mhm->next)
    {
      if ((mhm->attr_flags & MU_ATTRIBUTE_MODIFIED) ||
	  (mhm->attr_flags & MU_ATTRIBUTE_DELETED) ||
	  (mhm->message && message_is_modified (mhm->message)))
	break;
    }

  if (!mhm)
    return 0; /* Nothing changed, just return.  */

  while (mhm)
    {
      struct _mh_message *next = mhm->next;

      if (mhm->attr_flags & MU_ATTRIBUTE_DELETED)
	{
	  if (!mhm->deleted)
	    {
	      char *old_name, *new_name;
	      /* Rename original message */
	      old_name = _mh_message_name (mhm, 0);
	      new_name = _mh_message_name (mhm, 1);
	      rename (old_name, new_name);
	      free (old_name);
	      free (new_name);
	    }
	  _mh_message_delete (mhd, mhm);
	}
      else if (mhm->attr_flags & MU_ATTRIBUTE_MODIFIED)
	{
	  _mh_attach_message (mailbox, mhm, NULL);
	  mhm->deleted = mhm->attr_flags & MU_ATTRIBUTE_DELETED;
	  _mh_message_save (mhd, mhm, 1);
	}
      mhm = next;
    }

  return 0;
}

static int
mh_save_attributes (mailbox_t mailbox)
{
  /*FIXME*/
  (void)mailbox;
  return ENOSYS;
}

static int
mh_uidvalidity (mailbox_t mailbox, unsigned long *puidvalidity)
{
  struct _mh_data *mhd = mailbox->data;
  int status = mh_messages_count (mailbox, NULL);
  if (status != 0)
    return status;
  /* If we did not start a scanning yet do it now.  */
  if (mhd->msg_count == 0)
    {
      status = mh_scan0 (mailbox, 1, NULL, 0);
      if (status != 0)
	return status;
    }
  if (puidvalidity)
    *puidvalidity = mhd->uidvalidity;
  return 0;
}

static int
mh_uidnext (mailbox_t mailbox, size_t *puidnext)
{
  struct _mh_data *mhd = mailbox->data;
  int status = mh_messages_count (mailbox, NULL);
  if (status != 0)
    return status;
  /* If we did not start a scanning yet do it now.  */
  if (mhd->msg_count == 0)
    {
      status = mh_scan0 (mailbox, 1, NULL, 0);
      if (status != 0)
	return status;
    }
   if (puidnext)
     *puidnext = _mh_next_seq(mhd);
  return 0;
}

/* FIXME: effectively the same as mbox_cleanup */
static void
mh_cleanup (void *arg)
{
  mailbox_t mailbox = arg;
  monitor_unlock (mailbox->monitor);
  locker_unlock (mailbox->locker);
}

/* Insert message msg into the message list on the appropriate position */
static void
_mh_message_insert (struct _mh_data *mhd, struct _mh_message *msg)
{
  struct _mh_message *p;
  struct _mh_message *prev;
  size_t n = msg->seq_number;

  for (p = mhd->msg_head; p && p->seq_number < n; p = p->next)
    ;

  if (!p)
    {
      msg->next = NULL;
      msg->prev = mhd->msg_tail;
      mhd->msg_tail = msg;
      if (!mhd->msg_head)
	mhd->msg_head = msg;
    }
  else
    {
      msg->next = p;
      msg->prev = p->prev;
      p->prev = msg;
    }
  if ((prev = msg->prev) != NULL)
    prev->next = msg;
  else
    mhd->msg_head = msg;
  msg->mhd = mhd;
  mhd->msg_count++;
}

static void
_mh_message_delete (struct _mh_data *mhd, struct _mh_message *msg)
{
  struct _mh_message *p;

  if ((p = msg->next) != NULL)
    p->prev = msg->prev;
  else
    mhd->msg_tail = msg->prev;

  if ((p = msg->prev) != NULL)
    p->next = msg->next;
  else
    mhd->msg_head = msg->next;

  message_destroy (&msg->message, msg);
  free (msg);
  mhd->msg_count--;
}

/* Scan given message and fill mh_message_t fields.
   NOTE: the function assumes mhm->stream != NULL. */
static int
mh_scan_message (struct _mh_message *mhm)
{
  stream_t stream = mhm->stream;
  char buf[1024];
  off_t off = 0;
  size_t n;
  int status;
  int in_header = 1;
  size_t hlines = 0;
  size_t blines = 0;
  size_t body_start = 0;

  /* Check if the message was modified after the last scan */
  if (mhm->mtime)
    {
      struct stat st;
      char *msg_name = _mh_message_name (mhm, mhm->deleted);

      if (stat (msg_name, &st) == 0 && st.st_mtime == mhm->mtime)
	{
	  /* Nothing to do */
	  free (msg_name);
	  return 0;
	}
      free (msg_name);
    }

  while ((status = stream_readline (stream, buf, sizeof (buf), off, &n) == 0)
	 && n != 0)
    {
      if (in_header)
	{
	  if (buf[0] == '\n')
	    {
	      in_header = 0;
	      body_start = off+1;
	    }
	  if (buf[n - 1] == '\n')
	    hlines++;

	  /* Process particular attributes */
	  if (strncasecmp (buf, "status:", 7) == 0)
	    {
	      int deleted = mhm->attr_flags & MU_ATTRIBUTE_DELETED;
	      string_to_flags (buf, &mhm->attr_flags);
	      mhm->attr_flags |= deleted;
	    }
	  else if (strncasecmp (buf, "x-imapbase:", 11) == 0)
	    {
	      char *p;
	      mhm->mhd->uidvalidity = strtoul (buf + 11, &p, 10);
	      /* second number is next uid. Ignored */
	    }
	}
      else
	{
	  if (buf[n - 1] == '\n')
	    blines++;
	}
      off += n;
    }

  if (!body_start)
    body_start = off;
  mhm->header_lines = hlines;
  mhm->body_lines = blines;
  mhm->body_start = body_start;
  mhm->body_end = off;
  return 0;
}

/* Scan the mailbox */
static int
mh_scan0 (mailbox_t mailbox, size_t msgno, size_t *pcount, int do_notify)
{
  struct _mh_data *mhd = mailbox->data;
  struct _mh_message *msg;
  DIR *dir;
  struct dirent *entry;
  int status = 0;
  struct stat st;
  (void)msgno;

  if (mhd == NULL)
    return EINVAL;

  dir = opendir (mhd->name);
  if (!dir)
    return errno;

  monitor_wrlock (mailbox->monitor);

#ifdef WITH_PTHREAD
  pthread_cleanup_push (mh_cleanup, (void *)mailbox);
#endif

  locker_lock (mailbox->locker);

  /* Do actual work. */

  while ((entry = readdir (dir)))
    {
      char *namep;
      int attr_flags;
      size_t num;

      attr_flags = 0;
      switch (entry->d_name[0])
	{
	case '.':
	  /* FIXME: .mh_sequences */
	  continue;
	case ',':
	  continue;
#if 0
	  attr_flags |= MU_ATTRIBUTE_DELETED;
	  namep = entry->d_name+1;
	  break;
#endif
	case '0':case '1':case '2':case '3':case '4':
	case '5':case '6':case '7':case '8':case '9':
	  namep = entry->d_name;
	  break;
	default:
	  /*FIXME: Invalid entry. Report? */
	  continue;
	}

      num = strtoul (namep, &namep, 10);
      if (namep[0])
	continue;

      msg = _mh_get_message_seq (mhd, num);
      if (!msg)
	{
	  msg = calloc (1, sizeof(*msg));

	  msg->seq_number = num;
	  msg->attr_flags = attr_flags;
	  msg->deleted = attr_flags & MU_ATTRIBUTE_DELETED;

	  _mh_message_insert (mhd, msg);

	}
      else
	{
	  msg->attr_flags = attr_flags;
	}
    }

  closedir (dir);

  if (do_notify)
    for (msg = mhd->msg_head; msg; msg = msg->next)
      {
	DISPATCH_ADD_MSG(mailbox, mhd);
      }
	  
  if (stat (mhd->name, &st) == 0)
    mhd->mtime = st.st_mtime;

  if (pcount)
    *pcount = mhd->msg_count;

  /* Reset the uidvalidity.  */
  if (mhd->msg_count > 0)
    {
      if (mhd->uidvalidity == 0)
	{
	  mhd->uidvalidity = (unsigned long)time (NULL);
	  /* FIXME mhd->uidnext = mhd->msg_count + 1;*/
	  /* Tell that we have been modified for expunging.  */
	  if (mhd->msg_head)
	    {
	      mh_message_stream_open (mhd->msg_head);
	      mh_message_stream_close (mhd->msg_head);
	      mhd->msg_head->attr_flags |= MU_ATTRIBUTE_MODIFIED;
	    }
	}
    }

  /* Clean up the things */

  mh_cleanup (mailbox);
#ifdef WITH_PTHREAD
  pthread_cleanup_pop (0);
#endif
  return status;
}


static int
mh_scan (mailbox_t mailbox, size_t msgno, size_t *pcount)
{
  struct _mh_data *mhd = mailbox->data;

  if (! mh_is_updated (mailbox))
    return mh_scan0 (mailbox, msgno, pcount, 1);

  if (pcount)
    *pcount = mhd->msg_count;

  return 0;
}

/* Is the internal representation of the mailbox up to date.
   Return 1 if so, 0 otherwise. */
static int
mh_is_updated (mailbox_t mailbox)
{
  struct stat st;
  struct _mh_data *mhd = mailbox->data;

  if (!mhd->msg_head)
    return 0;

  if (stat (mhd->name, &st) < 0)
    return 1;

  return mhd->mtime == st.st_mtime;
}

static int
mh_get_size (mailbox_t mailbox, off_t *psize)
{
  /*FIXME*/
  (void)mailbox;
  (void)psize;
  return ENOSYS;
}

/* Return number of open streams residing in a message pool */
static int
mh_pool_open_count(struct _mh_data *mhd)
{
  int cnt = mhd->pool_last - mhd->pool_first;
  if (cnt < 0)
    cnt += MAX_OPEN_STREAMS;
  return cnt;
}

/* Look up a _mh_message in the pool of open messages.
   Returns 1 if the message is found in the pool, and 0 otherwise. */
static int
mh_pool_lookup (struct _mh_message *mhm)
{
  struct _mh_data *mhd = mhm->mhd;
  int i;

  for (i = mhd->pool_first; i != mhd->pool_last; )
    {
      if (mhd->msg_pool[i] == mhm)
	return 1;
      if (++i == MAX_OPEN_STREAMS)
	i = 0;
    }
  return 0;
}

/* Open a stream associated with the message mhm. If the stream is
   already open, do nothing */
static int
mh_pool_open (struct _mh_message *mhm)
{
  struct _mh_data *mhd = mhm->mhd;
  if (mh_pool_lookup (mhm))
    return 0;
  if (mh_pool_open_count(mhd) == MAX_OPEN_STREAMS-1)
    {
      mh_message_stream_close (mhd->msg_pool[mhd->pool_first++]);
      mhd->pool_first %= MAX_OPEN_STREAMS;
    }
  mh_message_stream_open (mhm);
  mhd->msg_pool[mhd->pool_last++] = mhm;
  mhd->pool_last %= MAX_OPEN_STREAMS;
  return 0;
}

/* Attach a stream to a given message structure. The latter is supposed
   to be already added to the open message pool. */
int
mh_message_stream_open (struct _mh_message *mhm)
{
  struct _mh_data *mhd = mhm->mhd;
  char *filename = NULL;
  int status;

  filename = _mh_message_name (mhm, mhm->deleted);

  if (!filename)
    return ENOMEM;

  status = file_stream_create (&mhm->stream, filename,
			       mhd->mailbox->flags | MU_STREAM_ALLOW_LINKS);

  free (filename);

  if (status != 0)
    return status;

  status = stream_open (mhm->stream);

  if (status != 0)
    stream_destroy (&mhm->stream, NULL);

  if (status == 0)
    status = mh_scan_message (mhm);

  return status;
}

/* Close the stream associated with the given message. */
void
mh_message_stream_close (struct _mh_message *mhm)
{
  if (mhm)
    {
      stream_close (mhm->stream);
      mhm->stream = NULL;
    }
}

void
mh_check_message (struct _mh_message *mhm)
{
  if (mhm->body_end == 0)
    mh_pool_open (mhm);
}

/* Reading functions */

static int
mh_readstream (struct _mh_message *mhm, char *buffer, size_t buflen,
	       off_t off, size_t *pnread, int isreadline,
	       off_t start, off_t end)
{
  size_t nread = 0;
  int status = 0;
  off_t ln;

  if (buffer == NULL || buflen == 0)
    {
      if (pnread)
	*pnread = nread;
      return 0;
    }

  monitor_rdlock (mhm->mhd->mailbox->monitor);
#ifdef WITH_PTHREAD
  /* read() is cancellation point since we're doing a potentially
     long operation.  Lets make sure we clean the state.  */
  pthread_cleanup_push (mh_cleanup, (void *)mhm->mhd->mailbox);
#endif

  ln = end - (start + off);
  if (ln > 0)
    {
      /* Position the file pointer and the buffer.  */
      nread = ((size_t)ln < buflen) ? (size_t)ln : buflen;
      if (isreadline)
	status = stream_readline (mhm->stream, buffer, buflen,
				  start + off, &nread);
      else
	status = stream_read (mhm->stream, buffer, nread,
			      start + off, &nread);
    }

  monitor_unlock (mhm->mhd->mailbox->monitor);
#ifdef WITH_PTHREAD
  pthread_cleanup_pop (0);
#endif

  if (pnread)
    *pnread = nread;
  return status;
}

static int
mh_body_read (stream_t is, char *buffer, size_t buflen, off_t off,
	      size_t *pnread)
{
  body_t body = stream_get_owner (is);
  message_t msg = body_get_owner (body);
  struct _mh_message *mhm = message_get_owner (msg);
  mh_pool_open (mhm);
  return mh_readstream (mhm, buffer, buflen, off, pnread, 0,
			mhm->body_start, mhm->body_end);
}

static int
mh_body_readline (stream_t is, char *buffer, size_t buflen,
		  off_t off, size_t *pnread)
{
  body_t body = stream_get_owner (is);
  message_t msg = body_get_owner (body);
  struct _mh_message *mhm = message_get_owner (msg);
  mh_pool_open (mhm);
  return mh_readstream (mhm, buffer, buflen, off, pnread, 1,
			mhm->body_start, mhm->body_end);
}

/* Return corresponding sizes */

static int
mh_stream_size (stream_t stream, off_t *psize)
{
  body_t body = stream_get_owner (stream);
  return mh_body_size (body, (size_t*) psize);
}

static int
mh_body_size (body_t body, size_t *psize)
{
  message_t msg = body_get_owner (body);
  struct _mh_message *mhm = message_get_owner (msg);
  if (mhm == NULL)
    return EINVAL;
  mh_check_message (mhm);
  if (psize)
    *psize = mhm->body_end - mhm->body_start;
  return 0;
}

static int
mh_body_lines (body_t body, size_t *plines)
{
  message_t msg = body_get_owner (body);
  struct _mh_message *mhm = message_get_owner (msg);
  if (mhm == NULL)
    return EINVAL;
  mh_check_message (mhm);
  if (plines)
    *plines = mhm->body_lines;
  return 0;
}

static int
mh_message_uid (message_t msg, size_t *puid)
{
  struct _mh_message *mhm = message_get_owner (msg);
  if (puid)
    *puid = mhm->seq_number;
  return 0;
}

/* Headers */
static int
mh_header_fill (header_t header, char *buffer, size_t len,
		off_t off, size_t *pnread)
{
  message_t msg = header_get_owner (header);
  struct _mh_message *mhm = message_get_owner (msg);

  mh_pool_open (mhm);
  return mh_readstream (mhm, buffer, len, off, pnread, 0,
			0, mhm->body_start);
}

static int
mh_header_size (header_t header, size_t *psize)
{
  message_t msg = header_get_owner (header);
  struct _mh_message *mhm = message_get_owner (msg);
  if (mhm == NULL)
    return EINVAL;
  mh_check_message (mhm);
  if (psize)
    *psize = mhm->body_start;
  return 0;
}

static int
mh_header_lines (header_t header, size_t *plines)
{
  message_t msg = header_get_owner (header);
  struct _mh_message *mhm = message_get_owner (msg);
  if (mhm == NULL)
    return EINVAL;
  mh_check_message (mhm);
  if (plines)
    *plines = mhm->header_lines;
  return 0;
}

/* Attributes */
static int
mh_get_attr_flags (attribute_t attr, int *pflags)
{
  message_t msg = attribute_get_owner (attr);
  struct _mh_message *mhm = message_get_owner (msg);

  if (mhm == NULL)
    return EINVAL;
  if (pflags)
    *pflags = mhm->attr_flags;
  return 0;
}

static int
mh_set_attr_flags (attribute_t attr, int flags)
{
  message_t msg = attribute_get_owner (attr);
  struct _mh_message *mhm = message_get_owner (msg);

  if (mhm == NULL)
    return EINVAL;
  mhm->attr_flags |= flags;
  return 0;
}

static int
mh_unset_attr_flags (attribute_t attr, int flags)
{
  message_t msg = attribute_get_owner (attr);
  struct _mh_message *mhm = message_get_owner (msg);

  if (mhm == NULL)
    return EINVAL;
  mhm->attr_flags &= ~flags;
  return 0;
}

/* Envelope */
static int
mh_envelope_date (envelope_t envelope, char *buf, size_t len,
		  size_t *psize)
{
  message_t msg = envelope_get_owner (envelope);
  struct _mh_message *mhm = message_get_owner (msg);
  header_t hdr = NULL;
  char *from;
  int status;
  
  if (mhm == NULL)
    return EINVAL;

  if ((status = message_get_header (msg, &hdr)) != 0)
    return status;
  if (header_aget_value (hdr, MH_ENV_DATE_HEADER, &from))
    return ENOSYS;

  /* Format:  "sender date" */
  if (buf && len > 0)
    {
      len--; /* Leave space for the null.  */
      strncpy (buf, from, len);
      if (strlen (from) < len)
	{
	  len = strlen (buf);
	  buf[len++] = '\n';
	}
      buf[len] = '\0';
    }
  else
    len = 0;

  if (psize)
    *psize = len;
  return 0;
}

static int
mh_envelope_sender (envelope_t envelope, char *buf, size_t len, size_t *psize)
{
  message_t msg = envelope_get_owner (envelope);
  struct _mh_message *mhm = message_get_owner (msg);
  header_t hdr = NULL;
  char *from;
  int status;

  if (mhm == NULL)
    return EINVAL;

  if ((status = message_get_header (msg, &hdr)) != 0)
    return status;
  if (header_aget_value (hdr, MH_ENV_SENDER_HEADER, &from))
    return ENOSYS;

  if (buf && len > 0)
    {
      int slen = strlen (from);

      if (len < slen + 1)
	slen = len - 1;
      memcpy (buf, from, slen);
      buf[slen] = 0;
    }
  else
    len = 0;

  if (psize)
    *psize = len;
  return 0;
}

#endif
