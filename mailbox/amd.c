/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002, 2003,
   2004 Free Software Foundation, Inc.

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

/* Mailutils Abstract Mail Directory Layer 
   First draft by Sergey Poznyakoff.
   Thanks Tang Yong Ping <yongping.tang@radixs.com> for initial
   patch (although not used here).

   This module provides basic support for "MH" and "Maildir" formats. */

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
#include <url0.h>
#include <amd.h>

static void amd_destroy __P((mailbox_t mailbox));
static int amd_open __P ((mailbox_t, int));
static int amd_close __P ((mailbox_t));
static int amd_get_message __P ((mailbox_t, size_t, message_t *));
static int amd_append_message __P ((mailbox_t, message_t));
static int amd_messages_count __P ((mailbox_t, size_t *));
static int amd_messages_recent __P ((mailbox_t, size_t *));
static int amd_message_unseen __P ((mailbox_t, size_t *));
static int amd_expunge __P ((mailbox_t));
static int amd_save_attributes __P ((mailbox_t));
static int amd_uidnext __P((mailbox_t mailbox, size_t *puidnext));
static int amd_uidvalidity __P ((mailbox_t, unsigned long *));
static int amd_scan __P ((mailbox_t, size_t, size_t *));
static int amd_is_updated __P ((mailbox_t));
static int amd_get_size __P ((mailbox_t, off_t *));

static int amd_body_read __P ((stream_t, char *, size_t, off_t, size_t *));
static int amd_body_readline __P ((stream_t, char *, size_t, off_t, size_t *));
static int amd_stream_size __P ((stream_t stream, off_t *psize));

static int amd_body_size __P ((body_t body, size_t *psize));
static int amd_body_lines __P ((body_t body, size_t *plines));

static int amd_header_fill __P((header_t header, char *buffer, size_t len,
			       off_t off, size_t *pnread));
static int amd_header_size __P((header_t header, size_t *psize));
static int amd_header_lines __P((header_t header, size_t *plines));

static int amd_get_attr_flags __P((attribute_t attr, int *pflags));
static int amd_set_attr_flags __P((attribute_t attr, int flags));
static int amd_unset_attr_flags __P((attribute_t attr, int flags));

static void _amd_message_delete __P((struct _amd_data *amd,
				    struct _amd_message *msg));
static int amd_pool_open __P((struct _amd_message *mhm));
static int amd_pool_open_count __P((struct _amd_data *amd));
static struct _amd_message **amd_pool_lookup __P((struct _amd_message *mhm));

static int amd_envelope_date __P((envelope_t envelope, char *buf, size_t len,
				  size_t *psize));
static int amd_envelope_sender __P((envelope_t envelope, char *buf, size_t len,
				   size_t *psize));

/* Operations on message array */

/* Perform binary search for message MSG on a segment of message array
   of AMD between the indexes FIRST and LAST inclusively.
   If found, return 0 and store index of the located entry in the
   variable PRET. Otherwise, return 1 and place into PRET index of
   the nearest array element that is less than MSG (in the sense of
   amd->msg_cmp)
   Indexes are zero-based. */
   
static int
amd_msg_bsearch (struct _amd_data *amd, off_t first, off_t last,
		 struct _amd_message *msg,
		 off_t *pret)
{
  off_t mid;
  int rc;

  if (last < first)
    return 1;
  
  mid = (first + last) / 2;
  rc = amd->msg_cmp (amd->msg_array[mid], msg);
  if (rc > 0)
    return amd_msg_bsearch (amd, first, mid-1, msg, pret);
  *pret = mid;
  if (rc < 0)
    return amd_msg_bsearch (amd, mid+1, last, msg, pret);
  /* else */
  return 0;
}

/* Search for message MSG in the message array of AMD.
   If found, return 0 and store index of the located entry in the
   variable PRET. Otherwise, return 1 and place into PRET index of
   the array element that is less than MSG (in the sense of
   amd->msg_cmp)
   Index returned in PRET is 1-based, so *PRET == 0 means that MSG
   is less than the very first element of the message array.

   In other words, when amd_msg_lookup() returns 1, the value in *PRET
   can be regarded as a 0-based index of the array slot where MSG can
   be inserted */

int
amd_msg_lookup (struct _amd_data *amd, struct _amd_message *msg,
		 size_t *pret)
{
  int rc;
  off_t i;
  
  if (!amd->msg_array)
    {
      *pret = 0;
      return 1;
    }
  
  rc = amd->msg_cmp (msg, amd->msg_array[0]);
  if (rc < 0)
    {
      *pret = 0;
      return 1;
    }
  else if (rc == 0)
    {
      *pret = 1;
      return 0;
    }
  
  rc = amd->msg_cmp (msg, amd->msg_array[amd->msg_count - 1]);
  if (rc > 0)
    {
      *pret = amd->msg_count;
      return 1;
    }
  else if (rc == 0)
    {
      *pret = amd->msg_count;
      return 0;
    }
  
  rc = amd_msg_bsearch (amd, 0, amd->msg_count - 1, msg, &i);
  *pret = i + 1;
  return rc;
}

#define AMD_MSG_INC 64

/* Prepare the message array for insertion of a new message
   at position INDEX (zero based), by moving its contents
   one slot to the right. If necessary, expand the array by
   AMD_MSG_INC */
int
amd_array_expand (struct _amd_data *amd, size_t index)
{
  if (amd->msg_count == amd->msg_max)
    {
      struct _amd_message **p;
      
      amd->msg_max += AMD_MSG_INC; /* FIXME: configurable? */
      p = realloc (amd->msg_array, amd->msg_max * amd->msg_size);
      if (!p)
	{
	  amd->msg_max -= AMD_MSG_INC;
	  return ENOMEM;
	}
      amd->msg_array = p;
    }
  memmove (&amd->msg_array[index+1], &amd->msg_array[index],
	   (amd->msg_count-index) * amd->msg_size);
  amd->msg_count++;
  return 0;
}

/* Shrink the message array by removing element at INDEX-1 and
   shifting left by one position all the elements on the right of
   it. */
int
amd_array_shrink (struct _amd_data *amd, size_t index)
{
  memmove (&amd->msg_array[index-1], &amd->msg_array[index],
	   (amd->msg_count-index) * amd->msg_size);
  amd->msg_count--;
  return 0;
}


int
amd_init_mailbox (mailbox_t mailbox, size_t amd_size, struct _amd_data **pamd) 
{
  struct _amd_data *amd;
  size_t name_len;

  if (mailbox == NULL || amd_size < sizeof (*amd))
    return EINVAL;

  amd = mailbox->data = calloc (1, amd_size);
  if (mailbox->data == NULL)
    return ENOMEM;

  /* Back pointer.  */
  amd->mailbox = mailbox;

  url_get_path (mailbox->url, NULL, 0, &name_len);
  amd->name = calloc (name_len + 1, sizeof (char));
  if (amd->name == NULL)
    {
      free (amd);
      mailbox->data = NULL;
      return ENOMEM;
    }
  url_get_path (mailbox->url, amd->name, name_len + 1, NULL);

  /* Overloading the defaults.  */
  mailbox->_destroy = amd_destroy;

  mailbox->_open = amd_open;
  mailbox->_close = amd_close;

  /* Overloading of the entire mailbox object methods.  */
  mailbox->_get_message = amd_get_message;
  mailbox->_append_message = amd_append_message;
  mailbox->_messages_count = amd_messages_count;
  mailbox->_messages_recent = amd_messages_recent;
  mailbox->_message_unseen = amd_message_unseen;
  mailbox->_expunge = amd_expunge;
  mailbox->_save_attributes = amd_save_attributes;
  mailbox->_uidvalidity = amd_uidvalidity;
  mailbox->_uidnext = amd_uidnext;

  mailbox->_scan = amd_scan;
  mailbox->_is_updated = amd_is_updated;

  mailbox->_get_size = amd_get_size;

  MAILBOX_DEBUG1 (mailbox, MU_DEBUG_TRACE, "amd_init(%s)\n", amd->name);
  *pamd = amd;
  return 0;
}

static void
amd_destroy (mailbox_t mailbox)
{
  struct _amd_data *amd = mailbox->data;
  size_t i;
  
  if (!amd)
    return;

  monitor_wrlock (mailbox->monitor);
  for (i = 0; i < amd->msg_count; i++)
    {
      message_destroy (&amd->msg_array[i]->message, amd->msg_array[i]);
      free (amd->msg_array[i]);
    }
  free (amd->msg_array);
	
  if (amd->name)
    free (amd->name);

  free (amd);
  mailbox->data = NULL;
  monitor_unlock (mailbox->monitor);
}

static int
amd_open (mailbox_t mailbox, int flags)
{
  struct _amd_data *amd = mailbox->data;
  struct stat st;

  mailbox->flags = flags;
  if (stat (amd->name, &st) < 0)
    return errno;

  if (!S_ISDIR (st.st_mode))
    return EINVAL;

  amd->mtime = st.st_mtime;

  return 0;
}

static int
amd_close (mailbox_t mailbox)
{
  if (!mailbox)
    return EINVAL;
  return 0;
}

struct _amd_message *
_amd_get_message (struct _amd_data *amd, size_t msgno)
{
  msgno--;
  if (msgno >= amd->msg_count)
    return NULL;
  return amd->msg_array[msgno];
}

static int
_amd_attach_message (mailbox_t mailbox, struct _amd_message *mhm,
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
    header_set_fill (header, amd_header_fill, msg);
    header_set_size (header, amd_header_size, msg);
    header_set_lines (header, amd_header_lines, msg);
    /*FIXME:
    header_set_get_fvalue (header, amd_header_get_fvalue, msg);
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
    attribute_set_get_flags (attribute, amd_get_attr_flags, msg);
    attribute_set_set_flags (attribute, amd_set_attr_flags, msg);
    attribute_set_unset_flags (attribute, amd_unset_attr_flags, msg);
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
    stream_set_read (stream, amd_body_read, body);
    stream_set_readline (stream, amd_body_readline, body);
    stream_set_size (stream, amd_stream_size, body);
    body_set_stream (body, stream, msg);
    body_set_size (body, amd_body_size, msg);
    body_set_lines (body, amd_body_lines, msg);
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
    envelope_set_sender (envelope, amd_envelope_sender, msg);
    envelope_set_date (envelope, amd_envelope_date, msg);
    message_set_envelope (msg, envelope, mhm);
  }

  /* Set the UID.  */
  if (mhm->amd->message_uid)
    message_set_uid (msg, mhm->amd->message_uid, mhm);

  /* Attach the message to the mailbox mbox data.  */
  mhm->message = msg;
  message_set_mailbox (msg, mailbox, mhm);

  if (pmsg)
    *pmsg = msg;

  return 0;
}

static int
amd_get_message (mailbox_t mailbox, size_t msgno, message_t *pmsg)
{
  int status;
  struct _amd_data *amd = mailbox->data;
  struct _amd_message *mhm;

  /* Sanity checks.  */
  if (pmsg == NULL || amd == NULL)
    return EINVAL;

  /* If we did not start a scanning yet do it now.  */
  if (amd->msg_count == 0)
    {
      status = amd->scan0 (mailbox, 1, NULL, 0);
      if (status != 0)
	return status;
    }

  if ((mhm = _amd_get_message (amd, msgno)) == NULL)
    return EINVAL;
  return _amd_attach_message (mailbox, mhm, pmsg);
}

static FILE *
_amd_tempfile(struct _amd_data *amd, char **namep)
{
  int fd = mu_tempfile (amd->name, namep);
  if (fd == -1)
      return NULL;
  return fdopen (fd, "w");
}

static int
_amd_delim (char *str)
{
  if (str[0] == '-')
    {
      for (; *str == '-'; str++)
	;
      for (; *str == ' ' || *str == '\t'; str++)
	;
    }
  return str[0] == '\n';
}

static int
_amd_message_save (struct _amd_data *amd, struct _amd_message *mhm, int expunge)
{
  stream_t stream = NULL;
  char *name = NULL, *buf = NULL, *msg_name;
  size_t n, off = 0;
  size_t bsize;
  size_t nlines, nbytes;
  size_t new_body_start, new_header_lines;
  FILE *fp;
  message_t msg = mhm->message;
  header_t hdr;
  int status;
  attribute_t attr;
  body_t body;
  char buffer[512];
  envelope_t env = NULL;
  
  fp = _amd_tempfile (mhm->amd, &name);
  if (!fp)
    return errno;

  message_size (msg, &bsize);

  /* Try to allocate large buffer */
  for (; bsize > 1; bsize /= 2)
    if ((buf = malloc (bsize)))
      break;

  if (!bsize)
    return ENOMEM;

  /* Copy flags */
  message_get_header (msg, &hdr);
  header_get_stream (hdr, &stream);
  off = 0;
  nlines = nbytes = 0;
  while ((status = stream_readline (stream, buf, bsize, off, &n)) == 0
	 && n != 0)
    {
      if (_amd_delim(buf))
	break;

      if (!(strncasecmp (buf, "status:", 7) == 0
	    || strncasecmp (buf, "x-imapbase:", 11) == 0
	    || strncasecmp (buf, "x-uid:", 6) == 0
	    || strncasecmp (buf, MU_HEADER_ENV_DATE ":", sizeof (MU_HEADER_ENV_DATE)) == 0
	    || strncasecmp (buf, MU_HEADER_ENV_SENDER ":", sizeof (MU_HEADER_ENV_SENDER)) == 0))
	{
	  nlines++;
	  nbytes += fprintf (fp, "%s", buf);
	}
      
      off += n;
    }

  /* Add imapbase */
  if (amd->next_uid
      && (!amd->msg_array || (amd->msg_array[0] == mhm))) /*FIXME*/
    {
      nbytes += fprintf (fp, "X-IMAPbase: %lu %u\n",
			 (unsigned long) amd->uidvalidity,
			 (unsigned) amd->next_uid (amd));
      nlines++;
    }
  
  message_get_envelope (msg, &env);
  if (envelope_date (env, buffer, sizeof buffer, &n) == 0 && n > 0)
    {
      /* NOTE: buffer is terminated with \n */
      char *p = buffer;
      while (isspace (*p))
	p++;
      nbytes += fprintf (fp, "%s: %s", MU_HEADER_ENV_DATE, p);

      if (*p && p[strlen (p) - 1] != '\n')
	nbytes += fprintf (fp, "\n");
      
      nlines++;
    }
	  
  if (envelope_sender (env, buffer, sizeof buffer, &n) == 0 && n > 0)
    {
      fprintf (fp, "%s: %s\n", MU_HEADER_ENV_SENDER, buffer);
      nlines++;
    }
  
  /* Add status */
  message_get_attribute (msg, &attr);
  attribute_to_string (attr, buf, bsize, &n);
  if (n)
    {
      nbytes += fprintf (fp, "%s", buf);
      nlines++;
    }
  nbytes += fprintf (fp, "\n");
  nlines++;
  
  new_header_lines = nlines;
  new_body_start = nbytes;

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
      nbytes += n;
    }

  mhm->header_lines = new_header_lines;
  mhm->body_start = new_body_start;
  mhm->body_lines = nlines;
  mhm->body_end = nbytes;

  free (buf);
  fclose (fp);

  msg_name = amd->msg_file_name (mhm, mhm->deleted);
  rename (name, msg_name);
  free (name);
  free (msg_name);

  return 0;
}

static int
amd_append_message (mailbox_t mailbox, message_t msg)
{
  int status;
  struct _amd_data *amd = mailbox->data;
  struct _amd_message *mhm;

  if (!mailbox || !msg)
    return EINVAL;

  mhm = calloc (1, amd->msg_size);
  if (!mhm)
    return ENOMEM;
    
  /* If we did not start a scanning yet do it now.  */
  if (amd->msg_count == 0)
    {
      status = amd->scan0 (mailbox, 1, NULL, 0);
      if (status != 0)
	{
	  free (mhm);
	  return status;
	}
    }

  mhm->amd = amd;
  if (amd->msg_init_delivery)
    {
      status = amd->msg_init_delivery (amd, mhm);
      if (status)
	{
	  free (mhm);
	  return status;
	}
    }
  
  mhm->message = msg;
  status = _amd_message_save (amd, mhm, 0);
  mhm->message = NULL;
  /* Insert and re-scan the message */
  status = _amd_message_insert (amd, mhm);
  if (status)
    {
      free (mhm);
      return status;
    }

  if (amd->msg_finish_delivery)
    amd->msg_finish_delivery (amd, mhm);
      
  return status;
}

static int
amd_messages_count (mailbox_t mailbox, size_t *pcount)
{
  struct _amd_data *amd = mailbox->data;

  if (amd == NULL)
    return EINVAL;

  if (!amd_is_updated (mailbox))
    return amd->scan0 (mailbox,  amd->msg_count, pcount, 0);

  if (pcount)
    *pcount = amd->msg_count;

  return 0;
}

/* A "recent" message is the one not marked with MU_ATTRIBUTE_SEEN
   ('O' in the Status header), i.e. a message that is first seen
   by the current session (see attributes.h) */
static int
amd_messages_recent (mailbox_t mailbox, size_t *pcount)
{
  struct _amd_data *amd = mailbox->data;
  size_t count, i;

  /* If we did not start a scanning yet do it now.  */
  if (amd->msg_count == 0)
    {
      int status = amd->scan0 (mailbox, 1, NULL, 0);
      if (status != 0)
	return status;
    }
  count = 0;
  for (i = 0; i < amd->msg_count; i++)
    {
      if (MU_ATTRIBUTE_IS_UNSEEN(amd->msg_array[i]->attr_flags))
	count++;
    }
  *pcount = count;
  return 0;
}

/* An "unseen" message is the one that has not been read yet */
static int
amd_message_unseen (mailbox_t mailbox, size_t *pmsgno)
{
  struct _amd_data *amd = mailbox->data;
  size_t i;

  /* If we did not start a scanning yet do it now.  */
  if (amd->msg_count == 0)
    {
      int status = amd->scan0 (mailbox, 1, NULL, 0);
      if (status != 0)
	return status;
    }

  for (i = 0; i < amd->msg_count; i++)
    {
      if (MU_ATTRIBUTE_IS_UNREAD(amd->msg_array[0]->attr_flags))
	{
	  *pmsgno = i + 1;
	  break;
	}
    }
  return 0;
}

static int
amd_expunge (mailbox_t mailbox)
{
  struct _amd_data *amd = mailbox->data;
  struct _amd_message *mhm;
  size_t i;
  
  if (amd == NULL)
    return EINVAL;

  if (amd->msg_count == 0)
    return 0;

  /* Find the first dirty(modified) message.  */
  for (i = 0; i < amd->msg_count; i++)
    {
      mhm = amd->msg_array[i];
      if ((mhm->attr_flags & MU_ATTRIBUTE_MODIFIED) ||
	  (mhm->attr_flags & MU_ATTRIBUTE_DELETED) ||
	  (mhm->message && message_is_modified (mhm->message)))
	break;
    }

  while (i < amd->msg_count)
    {
      mhm = amd->msg_array[i];
      
      if (mhm->attr_flags & MU_ATTRIBUTE_DELETED)
	{
	  if (!mhm->deleted)
	    {
	      char *old_name, *new_name;
	      /* Rename original message */
	      old_name = amd->msg_file_name (mhm, 0);
	      new_name = amd->msg_file_name (mhm, 1);
	      rename (old_name, new_name);
	      free (old_name);
	      free (new_name);
	    }
	  _amd_message_delete (amd, mhm);
	  /* Do not increase i! */
	}
      else
	{
	  if ((mhm->attr_flags & MU_ATTRIBUTE_MODIFIED)
	      || (mhm->message && message_is_modified (mhm->message)))
	    {
	      _amd_attach_message (mailbox, mhm, NULL);
	      mhm->deleted = mhm->attr_flags & MU_ATTRIBUTE_DELETED;
	      _amd_message_save (amd, mhm, 1);
	    }
	  i++; /* Move to the next message */
	}
    }

  return 0;
}

static int
amd_save_attributes (mailbox_t mailbox)
{
  struct _amd_data *amd = mailbox->data;
  struct _amd_message *mhm;
  size_t i;
  
  if (amd == NULL)
    return EINVAL;

  if (amd->msg_count == 0)
    return 0;

  /* Find the first dirty(modified) message.  */
  for (i = 0; i < amd->msg_count; i++)
    {
      mhm = amd->msg_array[i];
      if ((mhm->attr_flags & MU_ATTRIBUTE_MODIFIED)
	  || (mhm->message && message_is_modified (mhm->message)))
	break;
    }

  for ( ; i < amd->msg_count; i++)
    {
      mhm = amd->msg_array[i];

      if ((mhm->attr_flags & MU_ATTRIBUTE_MODIFIED)
	  || (mhm->message && message_is_modified (mhm->message)))
	{
	  _amd_attach_message (mailbox, mhm, NULL);
	  mhm->deleted = mhm->attr_flags & MU_ATTRIBUTE_DELETED;
	  _amd_message_save (amd, mhm, 0);
	}
    }

  return 0;
}

static int
amd_uidvalidity (mailbox_t mailbox, unsigned long *puidvalidity)
{
  struct _amd_data *amd = mailbox->data;
  int status = amd_messages_count (mailbox, NULL);
  if (status != 0)
    return status;
  /* If we did not start a scanning yet do it now.  */
  if (amd->msg_count == 0)
    {
      status = amd->scan0 (mailbox, 1, NULL, 0);
      if (status != 0)
	return status;
    }
  if (puidvalidity)
    *puidvalidity = amd->uidvalidity;
  return 0;
}

static int
amd_uidnext (mailbox_t mailbox, size_t *puidnext)
{
  struct _amd_data *amd = mailbox->data;
  int status;
  
  if (!amd->next_uid)
    return ENOSYS;
  status = mailbox_messages_count (mailbox, NULL);
  if (status != 0)
    return status;
  /* If we did not start a scanning yet do it now.  */
  if (amd->msg_count == 0)
    {
      status = amd->scan0 (mailbox, 1, NULL, 0);
      if (status != 0)
	return status;
    }
   if (puidnext)
     *puidnext = amd->next_uid (amd);
  return 0;
}

/* FIXME: effectively the same as mbox_cleanup */
void
amd_cleanup (void *arg)
{
  mailbox_t mailbox = arg;
  monitor_unlock (mailbox->monitor);
  locker_unlock (mailbox->locker);
}

/* Insert message msg into the message list on the appropriate position */
int
_amd_message_insert (struct _amd_data *amd, struct _amd_message *msg)
{
  size_t index;

  if (amd_msg_lookup (amd, msg, &index))
    {
      /* Not found. Index is the index of the array cell where msg
	 must be placed */
      int rc = amd_array_expand (amd, index);
      if (rc)
	return rc;
      amd->msg_array[index] = msg;
      msg->amd = amd;
    }
  else
    {
      /*FIXME: Found? Shouldn't happen */
      return EEXIST;
    }
  return 0;
}

static void
_amd_message_delete (struct _amd_data *amd, struct _amd_message *msg)
{
  size_t index;
  struct _amd_message **pp;

  if (amd_msg_lookup (amd, msg, &index))
    {
      /* FIXME: Not found? */
      return;
    }

  msg = _amd_get_message (amd, index);

  pp = amd_pool_lookup (msg);
  if (pp)
    *pp = NULL;
  
  message_destroy (&msg->message, msg);
  if (amd->msg_free)
    amd->msg_free (msg);
  free (msg);
  amd_array_shrink (amd, index);
}

/* Scan given message and fill amd_message_t fields.
   NOTE: the function assumes mhm->stream != NULL. */
static int
amd_scan_message (struct _amd_message *mhm)
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
      char *msg_name = mhm->amd->msg_file_name (mhm, mhm->deleted);

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
	      mhm->amd->uidvalidity = strtoul (buf + 11, &p, 10);
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

static int
amd_scan (mailbox_t mailbox, size_t msgno, size_t *pcount)
{
  struct _amd_data *amd = mailbox->data;

  if (! amd_is_updated (mailbox))
    return amd->scan0 (mailbox, msgno, pcount, 1);

  if (pcount)
    *pcount = amd->msg_count;

  return 0;
}

/* Is the internal representation of the mailbox up to date.
   Return 1 if so, 0 otherwise. */
static int
amd_is_updated (mailbox_t mailbox)
{
  struct stat st;
  struct _amd_data *amd = mailbox->data;

  if (amd->msg_count == 0)
    return 0;

  if (stat (amd->name, &st) < 0)
    return 1;

  return amd->mtime == st.st_mtime;
}

static int
amd_get_size (mailbox_t mailbox ARG_UNUSED, off_t *psize ARG_UNUSED)
{
  /*FIXME*/
  return ENOSYS;
}

/* Return number of open streams residing in a message pool */
static int
amd_pool_open_count (struct _amd_data *amd)
{
  int cnt = amd->pool_last - amd->pool_first;
  if (cnt < 0)
    cnt += MAX_OPEN_STREAMS;
  return cnt;
}

/* Look up a _amd_message in the pool of open messages.
   If the message is found in the pool, returns the address of
   the pool slot occupied by it. Otherwise returns NULL. */
static struct _amd_message **
amd_pool_lookup (struct _amd_message *mhm)
{
  struct _amd_data *amd = mhm->amd;
  int i;

  for (i = amd->pool_first; i != amd->pool_last; )
    {
      if (amd->msg_pool[i] == mhm)
	return &amd->msg_pool[i];
      if (++i == MAX_OPEN_STREAMS)
	i = 0;
    }
  return NULL;
}

/* Open a stream associated with the message mhm. If the stream is
   already open, do nothing.
   NOTE: We could have reused the NULL holes in the msg_pool, but
   that hardly is worth the effort, since the holes appear only when
   expunging. On the other hand this may be useful when MAX_OPEN_STREAMS
   size is very big. "Premature optimization is the root of all evil" */
static int
amd_pool_open (struct _amd_message *mhm)
{
  struct _amd_data *amd = mhm->amd;
  if (amd_pool_lookup (mhm))
    return 0;
  if (amd_pool_open_count(amd) == MAX_OPEN_STREAMS-1)
    {
      amd_message_stream_close (amd->msg_pool[amd->pool_first++]);
      amd->pool_first %= MAX_OPEN_STREAMS;
    }
  amd_message_stream_open (mhm);
  amd->msg_pool[amd->pool_last++] = mhm;
  amd->pool_last %= MAX_OPEN_STREAMS;
  return 0;
}

/* Attach a stream to a given message structure. The latter is supposed
   to be already added to the open message pool. */
int
amd_message_stream_open (struct _amd_message *mhm)
{
  struct _amd_data *amd = mhm->amd;
  char *filename = amd->msg_file_name (mhm, mhm->deleted);
  int status;
  int flags = MU_STREAM_ALLOW_LINKS;
  
  if (!filename)
    return ENOMEM;

  /* The message should be at least readable */
  if (amd->mailbox->flags & (MU_STREAM_RDWR|MU_STREAM_WRITE|MU_STREAM_APPEND))
    flags |= MU_STREAM_RDWR;
  else 
    flags |= MU_STREAM_READ;
  status = file_stream_create (&mhm->stream, filename, flags);

  free (filename);

  if (status != 0)
    return status;

  status = stream_open (mhm->stream);

  if (status != 0)
    stream_destroy (&mhm->stream, NULL);

  if (status == 0)
    status = amd_scan_message (mhm);

  return status;
}

/* Close the stream associated with the given message. */
void
amd_message_stream_close (struct _amd_message *mhm)
{
  if (mhm)
    {
      stream_close (mhm->stream);
      mhm->stream = NULL;
    }
}

void
amd_check_message (struct _amd_message *mhm)
{
  if (mhm->body_end == 0)
    amd_pool_open (mhm);
}

/* Reading functions */

static int
amd_readstream (struct _amd_message *mhm, char *buffer, size_t buflen,
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

  monitor_rdlock (mhm->amd->mailbox->monitor);
#ifdef WITH_PTHREAD
  /* read() is cancellation point since we're doing a potentially
     long operation.  Lets make sure we clean the state.  */
  pthread_cleanup_push (amd_cleanup, (void *)mhm->amd->mailbox);
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

  monitor_unlock (mhm->amd->mailbox->monitor);
#ifdef WITH_PTHREAD
  pthread_cleanup_pop (0);
#endif

  if (pnread)
    *pnread = nread;
  return status;
}

static int
amd_body_read (stream_t is, char *buffer, size_t buflen, off_t off,
	      size_t *pnread)
{
  body_t body = stream_get_owner (is);
  message_t msg = body_get_owner (body);
  struct _amd_message *mhm = message_get_owner (msg);
  amd_pool_open (mhm);
  return amd_readstream (mhm, buffer, buflen, off, pnread, 0,
			mhm->body_start, mhm->body_end);
}

static int
amd_body_readline (stream_t is, char *buffer, size_t buflen,
		  off_t off, size_t *pnread)
{
  body_t body = stream_get_owner (is);
  message_t msg = body_get_owner (body);
  struct _amd_message *mhm = message_get_owner (msg);
  amd_pool_open (mhm);
  return amd_readstream (mhm, buffer, buflen, off, pnread, 1,
			mhm->body_start, mhm->body_end);
}

/* Return corresponding sizes */

static int
amd_stream_size (stream_t stream, off_t *psize)
{
  body_t body = stream_get_owner (stream);
  return amd_body_size (body, (size_t*) psize);
}

static int
amd_body_size (body_t body, size_t *psize)
{
  message_t msg = body_get_owner (body);
  struct _amd_message *mhm = message_get_owner (msg);
  if (mhm == NULL)
    return EINVAL;
  amd_check_message (mhm);
  if (psize)
    *psize = mhm->body_end - mhm->body_start;
  return 0;
}

static int
amd_body_lines (body_t body, size_t *plines)
{
  message_t msg = body_get_owner (body);
  struct _amd_message *mhm = message_get_owner (msg);
  if (mhm == NULL)
    return EINVAL;
  amd_check_message (mhm);
  if (plines)
    *plines = mhm->body_lines;
  return 0;
}

/* Headers */
static int
amd_header_fill (header_t header, char *buffer, size_t len,
		off_t off, size_t *pnread)
{
  message_t msg = header_get_owner (header);
  struct _amd_message *mhm = message_get_owner (msg);

  amd_pool_open (mhm);
  return amd_readstream (mhm, buffer, len, off, pnread, 0,
			0, mhm->body_start);
}

static int
amd_header_size (header_t header, size_t *psize)
{
  message_t msg = header_get_owner (header);
  struct _amd_message *mhm = message_get_owner (msg);
  if (mhm == NULL)
    return EINVAL;
  amd_check_message (mhm);
  if (psize)
    *psize = mhm->body_start;
  return 0;
}

static int
amd_header_lines (header_t header, size_t *plines)
{
  message_t msg = header_get_owner (header);
  struct _amd_message *mhm = message_get_owner (msg);
  if (mhm == NULL)
    return EINVAL;
  amd_check_message (mhm);
  if (plines)
    *plines = mhm->header_lines;
  return 0;
}

/* Attributes */
static int
amd_get_attr_flags (attribute_t attr, int *pflags)
{
  message_t msg = attribute_get_owner (attr);
  struct _amd_message *mhm = message_get_owner (msg);

  if (mhm == NULL)
    return EINVAL;
  if (pflags)
    *pflags = mhm->attr_flags;
  return 0;
}

static int
amd_set_attr_flags (attribute_t attr, int flags)
{
  message_t msg = attribute_get_owner (attr);
  struct _amd_message *mhm = message_get_owner (msg);

  if (mhm == NULL)
    return EINVAL;
  mhm->attr_flags |= flags;
  return 0;
}

static int
amd_unset_attr_flags (attribute_t attr, int flags)
{
  message_t msg = attribute_get_owner (attr);
  struct _amd_message *mhm = message_get_owner (msg);

  if (mhm == NULL)
    return EINVAL;
  mhm->attr_flags &= ~flags;
  return 0;
}

/* Envelope */
static int
amd_envelope_date (envelope_t envelope, char *buf, size_t len,
		  size_t *psize)
{
  message_t msg = envelope_get_owner (envelope);
  struct _amd_message *mhm = message_get_owner (msg);
  header_t hdr = NULL;
  char *from;
  int status;
  
  if (mhm == NULL)
    return EINVAL;

  if ((status = message_get_header (msg, &hdr)) != 0)
    return status;
  if (header_aget_value (hdr, MU_HEADER_ENV_DATE, &from)
      && header_aget_value (hdr, MU_HEADER_DELIVERY_DATE, &from))
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
amd_envelope_sender (envelope_t envelope, char *buf, size_t len, size_t *psize)
{
  message_t msg = envelope_get_owner (envelope);
  struct _amd_message *mhm = message_get_owner (msg);
  header_t hdr = NULL;
  char *from;
  int status;

  if (mhm == NULL)
    return EINVAL;

  if ((status = message_get_header (msg, &hdr)) != 0)
    return status;
  if (header_aget_value (hdr, MU_HEADER_ENV_SENDER, &from))
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

static void
amd_url_destroy (url_t url ARG_UNUSED)
{
}

int
amd_url_init (url_t url, const char *scheme)
{
  const char *name = url_to_string (url);
  const char *path_ptr = name;
  size_t len = strlen (name);
  size_t scheme_len = strlen (scheme);
    
  if (!name)
    return 0;

  if (mu_scheme_autodetect_p (name, &path_ptr))
    /* nothing */ ;
  /* reject the obvious */
  else if (strncmp (scheme, name, scheme_len) != 0
	   || len < scheme_len + 1) 
    return EINVAL;
  else
    path_ptr = name + scheme_len;
  
  /* TYPE */
  url->_destroy = amd_url_destroy;

  /* SCHEME */
  url->scheme = strdup (scheme);
  if (url->scheme == NULL)
    {
      amd_url_destroy (url);
      return ENOMEM;
    }

  /* PATH */
  url->path = strdup (path_ptr);
  if (url->path == NULL)
    {
      amd_url_destroy (url);
      return ENOMEM;
    }

  return 0;
}

