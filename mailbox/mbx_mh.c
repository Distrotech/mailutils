/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

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

/* First draft by Sergey Poznyakoff */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

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

#include <mailutils/message.h>
#include <mailutils/stream.h>
#include <mailutils/body.h>
#include <mailutils/header.h>
#include <mailutils/attribute.h>
#include <mailutils/error.h>
#include <registrar0.h>
#include <mailbox0.h>

#define MAX_OPEN_STREAMS 16

struct _mh_data;
struct _mh_message
{
  struct _mh_message *next;
  struct _mh_message *prev;

  stream_t stream;          /* Associated file stream */
  off_t body_start;         /* Offset of body start in the message file */
  off_t body_end;           /* Offset of body end (size of file, effectively */

  size_t seq_number;        /* message sequence number */
  size_t uid;               /* IMAP uid. */

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
  size_t uidnext;                /* Next available UID */

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
static int mh_scan0 __P ((mailbox_t mailbox, size_t msgno, size_t *pcount));
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
    snprintf (filename, len, "%s/,%d", mhm->mhd->name, mhm->seq_number);
  else
    snprintf (filename, len, "%s/%d", mhm->mhd->name, mhm->seq_number);
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
  mailbox->properties = calloc (1, sizeof (*mailbox->properties));
  if (mailbox->properties == NULL)
    return ENOMEM;
  mailbox->properties_count = 1;
  mailbox->properties[0].key = strdup ("TYPE");
  mailbox->properties[0].value = strdup ("MH");

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

  /* FIXME: something else? */
  if (mailbox->locker == NULL)
    status = locker_create (&mailbox->locker, mhd->name, strlen (mhd->name),
			    MU_LOCKER_PID | MU_LOCKER_FCNTL);
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
      status = mh_scan0 (mailbox, 1, NULL);
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
  char *filename;
  int fd;

  filename = malloc (strlen (mhd->name) + /*'/'*/1 + /* "muXXXXXX" */8 + 1);
  if (!filename)
    return NULL;
  sprintf (filename, "%s/muXXXXXX", mhd->name);

#ifdef HAVE_MKSTEMP
  {
    int save_mask = umask (077);
    fd = mkstemp (filename);
    umask (save_mask);
  }
#else
  if (mktemp (filename))
    fd = open (filename, O_CREAT|O_EXCL|O_RDWR, 0600);
  else
    fd = -1;
#endif

  if (fd == -1)
    {
      free (filename);
      return NULL;
    }

  *namep = filename;
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
	    || strncasecmp (buf, "x-iud:", 6) == 0))
	fprintf (fp, "%s", buf);
      off += n;
    }

  mhm->header_lines = nlines;
  mhm->body_start = off;

  /* Add imapbase */
  if (!mhd->msg_head || (mhd->msg_head == mhm)) /*FIXME*/
    fprintf (fp, "X-IMAPbase: %lu %u\n", mhd->uidvalidity, mhd->uidnext);

  /* Add status */
  message_get_attribute (msg, &attr);
  attribute_to_string (attr, buf, bsize, &n);
  fprintf (fp, "%s", buf);

  /* Add UID */
  fprintf (fp, "X-UID: %d\n", mhm->uid);
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

  mhm->mhd = mhd;
  mhm->seq_number = _mh_next_seq (mhd);
  mhm->uid = mhd->uidnext++;
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
    return mh_scan0 (mailbox,  mhd->msg_count, pcount);

  if (pcount)
    *pcount = mhd->msg_count;

  return 0;
}

static int
mh_messages_recent (mailbox_t mailbox, size_t *pcount)
{
  struct _mh_data *mhd = mailbox->data;
  struct _mh_message *mhm;
  size_t count;

  /* If we did not start a scanning yet do it now.  */
  if (mhd->msg_count == 0)
    {
      int status = mh_scan0 (mailbox, 1, NULL);
      if (status != 0)
	return status;
    }
  count = 0;
  for (mhm = mhd->msg_head; mhm; mhm = mhm->next)
    {
      if ((mhm->attr_flags == 0) ||
	  ! ((mhm->attr_flags & MU_ATTRIBUTE_SEEN)
	     && (mhm->attr_flags & MU_ATTRIBUTE_READ)))
	count++;
    }
  *pcount = count;
  return 0;
}

static int
mh_message_unseen (mailbox_t mailbox, size_t *pmsgno)
{
  struct _mh_data *mhd = mailbox->data;
  struct _mh_message *mhm;
  size_t i, unseen;

  /* If we did not start a scanning yet do it now.  */
  if (mhd->msg_count == 0)
    {
      int status = mh_scan0 (mailbox, 1, NULL);
      if (status != 0)
	return status;
    }
  for (unseen = i = 1, mhm = mhd->msg_head; mhm; i++, mhm = mhm->next)
    {
      if ((mhm->attr_flags == 0) ||
	  !((mhm->attr_flags & MU_ATTRIBUTE_SEEN)
	    && (mhm->attr_flags & MU_ATTRIBUTE_READ)))
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
      status = mh_scan0 (mailbox, 1, NULL);
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
      status = mh_scan0 (mailbox, 1, NULL);
      if (status != 0)
	return status;
    }
   if (puidnext)
     *puidnext = mhd->uidnext;
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

/* Reset the IMAP uids, if necessary. UID according to IMAP RFC is a 32 bit
   ascending number for each messages  */
static void
mh_reset_imap_uids (struct _mh_data *mhd)
{
  size_t uid;
  size_t ouid;
  struct _mh_message *msg;

  for (uid = ouid = 0, msg = mhd->msg_head; msg; msg = msg->next)
    {
      uid = msg->uid;
      if (uid <= ouid)
	{
	  uid = ouid + 1;
	  msg->uid = ouid = uid;
	  msg->attr_flags |= MU_ATTRIBUTE_MODIFIED;
	}
      else
	ouid = uid;
    }
  if (mhd->msg_count > 0 && uid >= mhd->uidnext)
    {
      mhd->uidnext = uid + 1;
      mhd->msg_head->attr_flags |= MU_ATTRIBUTE_MODIFIED;
    }
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
	      mhm->mhd->uidnext = strtoul (p, NULL, 10);
	    }
	  else if (strncasecmp (buf, "x-uid:", 6) == 0)
	    {
	      mhm->uid = strtoul (buf + 6, NULL, 10);
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
mh_scan0 (mailbox_t mailbox, size_t msgno, size_t *pcount)
{
  struct _mh_data *mhd = mailbox->data;
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

  locker_lock (mailbox->locker, MU_LOCKER_RDLOCK);

  /* Do actual work */
  while ((entry = readdir (dir)))
    {
      char *namep;
      int attr_flags;
      size_t num;
      struct _mh_message *msg;

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

	  /* This scans the message */
	  mh_message_stream_open (msg);
	  mh_message_stream_close (msg);
	}
      else
	{
	  msg->attr_flags = attr_flags;
	}
    }

  closedir (dir);

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
	  mhd->uidnext = mhd->msg_count + 1;
	  /* Tell that we have been modified for expunging.  */
	  if (mhd->msg_head)
	    mhd->msg_head->attr_flags |= MU_ATTRIBUTE_MODIFIED;
	}
    }

  mh_reset_imap_uids (mhd);

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
    return mh_scan0 (mailbox, msgno, pcount);

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

  if (file_stream_create (&mhm->stream))
    return errno;

  filename = _mh_message_name (mhm, mhm->deleted);

  if (!filename)
    return ENOMEM;

  status = stream_open (mhm->stream, filename, 0, mhd->mailbox->flags);

  free (filename);

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
  if (plines)
    *plines = mhm->body_lines;
  return 0;
}

static int
mh_message_uid (message_t msg, size_t *puid)
{
  struct _mh_message *mhm = message_get_owner (msg);
  if (puid)
    *puid = mhm->uid;
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
  size_t n = 0;
  int status;
  char buffer[512];
  char *s;

  if (mhm == NULL)
    return EINVAL;

  mh_pool_open (mhm);

  status = stream_readline (mhm->stream, buffer, sizeof(buffer), 0, &n);
  if (status != 0)
    {
      if (psize)
	*psize = 0;
      return status;
    }

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

  if (psize)
    *psize = len;
  return 0;
}

static int
mh_envelope_sender (envelope_t envelope, char *buf, size_t len, size_t *psize)
{
  message_t msg = envelope_get_owner (envelope);
  struct _mh_message *mhm = message_get_owner (msg);
  size_t n = 0;
  int status;
  char buffer[512];
  char *s;

  if (mhm == NULL)
    return EINVAL;

  mh_pool_open (mhm);

  status = stream_readline (mhm->stream, buffer, sizeof(buffer),
			    0, &n);
  if (status != 0)
    {
      if (psize)
	*psize = 0;
      return status;
    }

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

  if (psize)
    *psize = len;
  return 0;
}

int
mh_message_number (message_t msg, size_t *pnum)
{
  struct _mh_message *mhm = message_get_owner (msg);
  if (pnum)
    *pnum = mhm->seq_number;
  return 0;
}
