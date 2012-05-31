/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2011-2012 Free Software Foundation, Inc.

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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <string.h>

#include <mailutils/errno.h>
#include <mailutils/stream.h>
#include <mailutils/message.h>
#include <mailutils/folder.h>
#include <mailutils/assoc.h>
#include <mailutils/url.h>
#include <mailutils/io.h>
#include <mailutils/nls.h>
#include <mailutils/diag.h>
#include <mailutils/filter.h>
#include <mailutils/observer.h>
#include <mailutils/envelope.h>
#include <mailutils/address.h>
#include <mailutils/attribute.h>
#include <mailutils/header.h>
#include <mailutils/body.h>
#include <mailutils/msgset.h>
#include <mailutils/sys/imap.h>

#define _imap_mbx_clrerr(imbx) ((imbx)->last_error = 0)
#define _imap_mbx_errno(imbx) ((imbx)->last_error)
#define _imap_mbx_uptodate(imbx) ((imbx)->flags & _MU_IMAP_MBX_UPTODATE)

static int _imap_mbx_scan (mu_mailbox_t mbox, size_t msgno, size_t *pcount);
static int _imap_mbx_is_updated (mu_mailbox_t mbox);

/* ------------------------------- */
/* Auxiliary message functions     */
/* ------------------------------- */

static size_t
_imap_msg_no (struct _mu_imap_message *imsg)
{
  return imsg - imsg->imbx->msgs + 1;
}

static int
_imap_fetch_with_callback (mu_imap_t imap, mu_msgset_t msgset, char *items,
			   mu_imap_callback_t cb, void *data)
{
  int rc;
  
  mu_imap_register_callback_function (imap, MU_IMAP_CB_FETCH, cb, data);
  rc = mu_imap_fetch (imap, 0, msgset, items);
  mu_imap_register_callback_function (imap, MU_IMAP_CB_FETCH, NULL, NULL);
  return rc;
}

static void
_imap_msg_free (struct _mu_imap_message *msg)
{
  mu_message_imapenvelope_free (msg->env);
  mu_message_destroy (&msg->message, msg);
}

struct save_closure
{
  mu_stream_t save_stream;
  size_t size;
  struct _mu_imap_message *imsg;
};

static int
_save_message_parser (void *item, void *data)
{
  union mu_imap_fetch_response *resp = item;
  struct save_closure *clos = data;

  if (resp->type == MU_IMAP_FETCH_BODY)
    {
      struct _mu_imap_message *imsg = clos->imsg;
      struct _mu_imap_mailbox *imbx = imsg->imbx;
      int rc;
      mu_stream_t istr, flt;
      mu_off_t size;
  
      rc = mu_static_memory_stream_create (&istr, resp->body.text,
					   strlen (resp->body.text));
      
      if (rc)
	{
	  mu_debug (MU_DEBCAT_MAILBOX, MU_DEBUG_ERROR,
		    (_("mu_static_memory_stream_create: %s"),
		     mu_strerror (rc)));
	  imbx->last_error = rc;
	  return 0;
	}

      rc = mu_filter_create (&flt, istr, "CRLF", MU_FILTER_DECODE,
			     MU_STREAM_READ);
      mu_stream_unref (istr);
      if (rc)
	{
	  mu_debug (MU_DEBCAT_MAILBOX, MU_DEBUG_ERROR,
		    (_("mu_filter_create: %s"), mu_strerror (rc)));
	  imbx->last_error = rc;
	  return 0;
	}
      
      rc = mu_stream_copy (clos->save_stream, flt, 0, &size);
      mu_stream_destroy (&flt);
      if (rc)
	{
	  mu_debug (MU_DEBCAT_MAILBOX, MU_DEBUG_ERROR,
		    (_("copying to cache failed: %s"), mu_strerror (rc)));
	  imbx->last_error = rc;
	  return 0;
	}
      clos->size = size;
    }
  else
    mu_debug (MU_DEBCAT_MAILBOX, MU_DEBUG_TRACE0,
	      (_("fetch returned a not requested item %d"),
	       resp->type));
  return 0;
}

static void
_save_message (void *data, int code, size_t sdat, void *pdat)
{
  mu_list_t list = pdat;
  mu_list_foreach (list, _save_message_parser, data);
}

static int
__imap_msg_get_stream (struct _mu_imap_message *imsg, size_t msgno,
		       mu_stream_t *pstr)
{
  int rc;
  struct _mu_imap_mailbox *imbx = imsg->imbx;
  mu_folder_t folder = imbx->mbox->folder;
  mu_imap_t imap = folder->data;
  
  if (!(imsg->flags & _MU_IMAP_MSG_CACHED))
    {
      mu_msgset_t msgset;
      
      mu_debug (MU_DEBCAT_MAILBOX, MU_DEBUG_TRACE1,
		(_("caching message %lu"), (unsigned long) msgno));
      if (!imbx->cache)
	{
	  rc = mu_temp_file_stream_create (&imbx->cache, NULL, 0);
	  if (rc)
	    /* FIXME: Try to recover first */
	    return rc;

	  mu_stream_set_buffer (imbx->cache, mu_buffer_full, 8192);
	}

      rc = mu_stream_size (imbx->cache, &imsg->offset);
      if (rc)
	return rc;

      rc = mu_msgset_create (&msgset, NULL, MU_MSGSET_NUM);
      if (rc == 0)
	{
	  struct save_closure clos;

	  clos.imsg = imsg;
	  clos.save_stream = imbx->cache;

	  rc = mu_msgset_add_range (msgset, msgno, msgno, MU_MSGSET_NUM);
	  if (rc == 0)
	    {
	      _imap_mbx_clrerr (imbx);
	      rc = _imap_fetch_with_callback (imap, msgset, "BODY[]",
					      _save_message, &clos);
	    }
	  mu_msgset_free (msgset);
	  if (rc == 0 && !_imap_mbx_errno (imbx))
	    {
	      mu_debug (MU_DEBCAT_MAILBOX, MU_DEBUG_TRACE1,
			(_("cached message %lu: offset=%lu, size=%lu"),
			 (unsigned long) msgno,
			 (unsigned long) imsg->offset,
			 (unsigned long) clos.size));
	      imsg->message_size = clos.size;
	    }
	}

      if (rc)
	return rc;

      imsg->flags |= _MU_IMAP_MSG_CACHED;
    }
  return mu_streamref_create_abridged (pstr, imbx->cache,
				       imsg->offset,
				       imsg->offset + imsg->message_size - 1);
}

static int
_imap_msg_scan (struct _mu_imap_message *imsg)
{
  int rc;
  mu_stream_t stream;
  struct mu_message_scan scan;
  size_t msgno = _imap_msg_no (imsg);
  
  if (imsg->flags & _MU_IMAP_MSG_SCANNED)
    return 0;
  
  rc = __imap_msg_get_stream (imsg, msgno, &stream);
  if (rc)
    return rc;
  
  scan.flags = MU_SCAN_SEEK | MU_SCAN_SIZE;
  scan.message_start = 0;
  scan.message_size = imsg->message_size;
  rc = mu_stream_scan_message (stream, &scan);
  mu_stream_unref (stream);

  if (rc == 0)
    {
      imsg->body_start = scan.body_start;
      imsg->body_end = scan.body_end;
      imsg->header_lines = scan.header_lines;
      imsg->body_lines = scan.body_lines;
      imsg->message_lines = imsg->header_lines + 1 + imsg->body_lines;
      imsg->flags |= _MU_IMAP_MSG_SCANNED;
    }
  
  return rc;
}

/* ------------------------------- */
/* Message envelope                */
/* ------------------------------- */

static int
_imap_env_date (mu_envelope_t env, char *buf, size_t len,
		size_t *pnwrite)
{
  struct _mu_imap_message *imsg = mu_envelope_get_owner (env);
  mu_stream_t str;
  int rc;
  
  if (!buf)
    rc = mu_nullstream_create (&str, MU_STREAM_WRITE);
  else
    rc = mu_fixed_memory_stream_create (&str, buf, len, MU_STREAM_WRITE);
  if (rc == 0)
    {
      mu_stream_stat_buffer statbuf;
      mu_stream_set_stat (str, MU_STREAM_STAT_MASK (MU_STREAM_STAT_OUT),
			  statbuf);
      rc = mu_c_streamftime (str, MU_DATETIME_FROM,
			     &imsg->env->date, &imsg->env->tz);
      if (rc == 0)
	rc = mu_stream_write (str, "", 1, NULL);
      mu_stream_destroy (&str);
      if (rc == 0 && pnwrite)
	/* Do not count terminating null character */
	*pnwrite = statbuf[MU_STREAM_STAT_OUT] - 1;
    }
  return rc;
}

static int
_imap_env_sender (mu_envelope_t env, char *buf, size_t len,
		  size_t *pnwrite)
{
  struct _mu_imap_message *imsg = mu_envelope_get_owner (env);
  mu_address_t addr = imsg->env->sender ? imsg->env->sender : imsg->env->from;

  if (!addr)
    return MU_ERR_NOENT;
  return mu_address_get_email (addr, 1, buf, len, pnwrite);
}
  
static int
_imap_msg_env_setup (struct _mu_imap_message *imsg, mu_message_t message)
{
  mu_envelope_t env;
  int rc = mu_envelope_create (&env, imsg);
  if (rc == 0)
    {
      mu_envelope_set_sender (env, _imap_env_sender, imsg);
      mu_envelope_set_date (env, _imap_env_date, imsg);
      rc = mu_message_set_envelope (message, env, imsg);
    }
  return rc;
}

/* ------------------------------- */
/* Message attributes              */
/* ------------------------------- */
static int
_imap_attr_get_flags (mu_attribute_t attr, int *pflags)
{
  struct _mu_imap_message *imsg = mu_attribute_get_owner (attr);

  if (!imsg)
    return EINVAL;
  if (pflags)
    *pflags = imsg->attr_flags;
  return 0;
}

static int
_imap_attr_set_flags (mu_attribute_t attr, int flags)
{
  struct _mu_imap_message *imsg = mu_attribute_get_owner (attr);

  if (!imsg)
    return EINVAL;
  imsg->attr_flags |= flags;
  imsg->flags |= _MU_IMAP_MSG_ATTRCHG;
  return 0;
}

static int
_imap_attr_clr_flags (mu_attribute_t attr, int flags)
{
  struct _mu_imap_message *imsg = mu_attribute_get_owner (attr);

  if (!imsg)
    return EINVAL;
  imsg->attr_flags |= flags;
  imsg->flags |= _MU_IMAP_MSG_ATTRCHG;
  return 0;
}

int
_imap_msg_attr_setup (struct _mu_imap_message *imsg, mu_message_t message)
{
  mu_attribute_t attribute;
  int rc = mu_attribute_create (&attribute, imsg);
  if (rc == 0)
    {
      mu_attribute_set_get_flags (attribute, _imap_attr_get_flags, imsg);
      mu_attribute_set_set_flags (attribute, _imap_attr_set_flags, imsg);
      mu_attribute_set_unset_flags (attribute, _imap_attr_clr_flags, imsg);
      rc = mu_message_set_attribute (message, attribute, imsg);
    }
  return rc;
}

/* ------------------------------- */
/* Header functions                */
/* ------------------------------- */
static int
_imap_hdr_fill (void *data, char **pbuf, size_t *plen)
{
  struct _mu_imap_message *imsg = data;
  struct _mu_imap_mailbox *imbx = imsg->imbx;
  mu_imap_t imap = imbx->mbox->folder->data;
  struct save_closure clos;
  mu_msgset_t msgset;
  unsigned long msgno = _imap_msg_no (imsg);
  int rc;

  rc = mu_msgset_create (&msgset, NULL, MU_MSGSET_NUM);
  if (rc == 0)
    {
      rc = mu_msgset_add_range (msgset, msgno, msgno, MU_MSGSET_NUM);
      if (rc == 0)
	{
	  clos.imsg = imsg;
	  rc = mu_memory_stream_create (&clos.save_stream, MU_STREAM_RDWR);
	  if (rc == 0)
	    {
	      mu_debug (MU_DEBCAT_MAILBOX, MU_DEBUG_TRACE1,
			(_("message %lu: reading headers"),
			 (unsigned long) msgno));
	      _imap_mbx_clrerr (imbx);
	      rc = _imap_fetch_with_callback (imap, msgset,
					      "BODY.PEEK[HEADER]",
					      _save_message, &clos);
	      if (rc == 0)
		{
		  char *buf;
		  mu_off_t size;
	  
		  mu_stream_size (clos.save_stream, &size);
		  buf = malloc (size + 1);
		  if (!buf)
		    rc = ENOMEM;
		  else
		    {
		      mu_stream_seek (clos.save_stream, 0, MU_SEEK_SET, NULL);
		      rc = mu_stream_read (clos.save_stream, buf, size, NULL);
		      if (rc == 0)
			{
			  *pbuf = buf;
			  *plen = size;
			}
		      else
			free (buf);
		    }
		}
	      mu_stream_destroy (&clos.save_stream);
	    }
	}
      mu_msgset_free (msgset);
    }
  return rc;
}

static int
_imap_msg_header_setup (struct _mu_imap_message *imsg, mu_message_t message)
{
  int rc;
  mu_header_t header = NULL;

  rc = mu_header_create (&header, NULL, 0);
  if (rc)
    return rc;
  mu_header_set_fill (header, _imap_hdr_fill, imsg);
  return mu_message_set_header (message, header, imsg);
}

/* ------------------------------- */
/* Body functions                  */
/* ------------------------------- */
static int
_imap_body_get_stream (mu_body_t body, mu_stream_t *pstr)
{
  mu_message_t msg = mu_body_get_owner (body);
  struct _mu_imap_message *imsg = mu_message_get_owner (msg);
  struct _mu_imap_mailbox *imbx = imsg->imbx;
  int rc;

  rc = _imap_msg_scan (imsg);
  if (rc)
    return rc;
  return mu_streamref_create_abridged (pstr, imbx->cache,
				       imsg->offset + imsg->body_start,
				       imsg->offset + imsg->body_end - 1);
}

static int
_imap_body_size (mu_body_t body, size_t *psize)
{
  mu_message_t msg = mu_body_get_owner (body);
  struct _mu_imap_message *imsg = mu_message_get_owner (msg);
  int rc;

  rc = _imap_msg_scan (imsg);
  if (rc)
    return rc;
  *psize = imsg->body_end - imsg->body_start;
  return 0;
}

static int
_imap_body_lines (mu_body_t body, size_t *psize)
{
  mu_message_t msg = mu_body_get_owner (body);
  struct _mu_imap_message *imsg = mu_message_get_owner (msg);
  int rc;

  rc = _imap_msg_scan (imsg);
  if (rc)
    return rc;
  *psize = imsg->body_lines;
  return 0;
}

static int
_imap_mbx_body_setup (struct _mu_imap_message *imsg, mu_message_t message)
{
  int rc;
  mu_body_t body;

  /* FIXME: The owner of the body *must* be the message it belongs to. */
  rc = mu_body_create (&body, message);
  if (rc)
    return rc;

  mu_body_set_get_stream (body, _imap_body_get_stream, message);
  mu_body_set_size (body, _imap_body_size, message);
  mu_body_set_lines (body, _imap_body_lines, message);

  return mu_message_set_body (message, body, imsg);
}


/* ------------------------------- */
/* Message functions               */
/* ------------------------------- */

static int
_imap_msg_get_stream (mu_message_t msg, mu_stream_t *pstr)
{
  struct _mu_imap_message *imsg = mu_message_get_owner (msg);
  return __imap_msg_get_stream (imsg, _imap_msg_no (imsg), pstr);
}

static int
_imap_msg_size (mu_message_t msg, size_t *psize)
{
  struct _mu_imap_message *imsg = mu_message_get_owner (msg);
  *psize = imsg->message_size;
  return 0;
}

static int
_imap_msg_lines (mu_message_t msg, size_t *plines, int quick)
{
  struct _mu_imap_message *imsg = mu_message_get_owner (msg);
  struct _mu_imap_mailbox *imbx = imsg->imbx;
  mu_mailbox_t mbox = imbx->mbox;
  
  if (!(imsg->flags & _MU_IMAP_MSG_LINES))
    {
      int rc;
      
      if (quick && !(imsg->flags & _MU_IMAP_MSG_CACHED))
	return MU_ERR_INFO_UNAVAILABLE;
      if (!_imap_mbx_uptodate (imbx))
	_imap_mbx_scan (mbox, 1, NULL);
      rc = _imap_msg_scan (imsg);
      if (rc)
	return rc;
    }
  *plines = imsg->message_lines;
  return 0;
}

static int
_copy_imapenvelope (struct mu_imapenvelope *env,
		    struct mu_imapenvelope const *src)
{  
  env->date = src->date;
  env->tz   = src->tz;
  
  if (src->subject && (env->subject = strdup (src->subject)) == NULL)
    return ENOMEM;
  if (src->from && (env->from = mu_address_dup (src->from)) == NULL)
    return ENOMEM;
  if (src->sender && (env->sender = mu_address_dup (src->sender)) == NULL)
    return ENOMEM;
  if (src->reply_to && (env->reply_to = mu_address_dup (src->reply_to))
      == NULL)
    return ENOMEM;
  if (src->to && (env->to = mu_address_dup (src->to)) == NULL)
    return ENOMEM;
  if (src->cc && (env->cc = mu_address_dup (src->cc)) == NULL)
    return ENOMEM;
  if (src->bcc && (env->bcc = mu_address_dup (src->bcc)) == NULL)
    return ENOMEM;
  if (src->in_reply_to && (env->in_reply_to = strdup (src->in_reply_to))
      == NULL)
    return ENOMEM;
  if (src->message_id && (env->message_id = strdup (src->message_id)) == NULL)
    return ENOMEM;
  return 0;
}
		   
static int
_imap_msg_imapenvelope (mu_message_t msg, struct mu_imapenvelope **penv)
{
  struct _mu_imap_message *imsg = mu_message_get_owner (msg);
  int rc = 0;
  struct mu_imapenvelope *env = calloc (1, sizeof (*env));

  if (!env)
    return ENOMEM;
  rc = _copy_imapenvelope (env, imsg->env);
  if (rc)
    mu_message_imapenvelope_free (env);
  else
    *penv = env;
  return rc;
}

static int
fetch_bodystructure_parser (void *item, void *data)
{
  union mu_imap_fetch_response *resp = item;
  struct mu_bodystructure **pbs = data;

  if (resp->type == MU_IMAP_FETCH_BODYSTRUCTURE)
    {
      *pbs = resp->bodystructure.bs;
      resp->bodystructure.bs = NULL;
    }
  else
    mu_debug (MU_DEBCAT_MAILBOX, MU_DEBUG_TRACE0,
	      (_("fetch returned a not requested item %d"),
	       resp->type));
  return 0;
}

static void
_imap_bodystructure_callback (void *data, int code, size_t sdat, void *pdat)
{
  mu_list_t list = pdat;
  mu_list_foreach (list, fetch_bodystructure_parser, data);
}
  
static int
_imap_msg_bodystructure (mu_message_t msg, struct mu_bodystructure **pbs)
{
  struct _mu_imap_message *imsg = mu_message_get_owner (msg);
  struct _mu_imap_mailbox *imbx = imsg->imbx;
  mu_imap_t imap = imbx->mbox->folder->data;
  int rc;
  mu_msgset_t msgset;

  rc = mu_msgset_create (&msgset, NULL, MU_MSGSET_NUM);
  if (rc == 0)
    {
      size_t msgno = _imap_msg_no (imsg);
      rc = mu_msgset_add_range (msgset, msgno, msgno, MU_MSGSET_NUM);
      if (rc == 0)
	rc = _imap_fetch_with_callback (imap, msgset, "BODYSTRUCTURE",
					_imap_bodystructure_callback, pbs);
      mu_msgset_free (msgset);
    }
  return rc;
}

static int
_imap_mbx_get_message (mu_mailbox_t mailbox, size_t msgno, mu_message_t *pmsg)
{
  struct _mu_imap_mailbox *imbx = mailbox->data;
  struct _mu_imap_message *imsg;
  int rc;
  
  /* If we did not start a scanning yet do it now.  */
  if (!_imap_mbx_uptodate (imbx))
    _imap_mbx_scan (mailbox, 1, NULL);

  if (msgno > imbx->msgs_cnt)
    return MU_ERR_NOENT;

  imsg = imbx->msgs + msgno - 1;
  if (!imsg->message)
    {
      mu_message_t msg;
  
      rc = mu_message_create (&msg, imsg);
      if (rc)
	return rc;
      mu_message_set_get_stream (msg, _imap_msg_get_stream, imsg);
      mu_message_set_size (msg, _imap_msg_size, imsg);
      mu_message_set_lines (msg, _imap_msg_lines, imsg);
      mu_message_set_imapenvelope (msg, _imap_msg_imapenvelope, imsg);
      mu_message_set_bodystructure (msg, _imap_msg_bodystructure, imsg);
      
      do
	{
	  rc = _imap_msg_env_setup (imsg, msg);
	  if (rc)
	    break;
	  rc = _imap_msg_attr_setup (imsg, msg);
	  if (rc)
	    break;
	  rc = _imap_msg_header_setup (imsg, msg);
	  if (rc)
	    break;
	  rc = _imap_mbx_body_setup (imsg, msg);
	}
      while (0);
      
      if (rc)
	{
	  mu_message_destroy (&msg, imsg);
	  return rc;
	}
      imsg->message = msg;

    }
  *pmsg = imsg->message;
  return 0;
}

/* ------------------------------- */
/* Mailbox functions               */
/* ------------------------------- */
static int
_imap_realloc_messages (struct _mu_imap_mailbox *imbx, size_t count)
{
  if (count > imbx->msgs_max)
    {
      struct _mu_imap_message *newmsgs = realloc (imbx->msgs,
						  count * sizeof (*newmsgs));
      if (!newmsgs)
	return ENOMEM;
      memset (newmsgs + imbx->msgs_max, 0,
	      sizeof (*newmsgs) * (count - imbx->msgs_max));
      imbx->msgs = newmsgs;
      imbx->msgs_max = count;
    }
  return 0;
}

static void
_imap_mbx_destroy (mu_mailbox_t mailbox)
{
  size_t i;
  struct _mu_imap_mailbox *imbx = mailbox->data;
  
  if (!imbx)
    return;

  if (imbx->msgs)
    {
      for (i = 0; i < imbx->msgs_cnt; i++)
	_imap_msg_free (imbx->msgs + i);
      free (imbx->msgs);
    }
  mu_stream_unref (imbx->cache);
  free (imbx);
  mailbox->data = NULL;
}

static void
_imap_update_callback (void *data, int code, size_t sdat, void *pdat)
{
  struct _mu_imap_mailbox *imbx = data;
  memcpy (&imbx->stats, pdat, sizeof (imbx->stats));
  imbx->flags &= ~_MU_IMAP_MBX_UPTODATE;
}

static int
_imap_mbx_open (mu_mailbox_t mbox, int flags)
{
  struct _mu_imap_mailbox *imbx = mbox->data;
  mu_folder_t folder = mbox->folder;
  int rc;
  const char *mbox_name;
  mu_url_t url;
  mu_imap_t imap;
  
  mbox->flags = flags;
  
  rc = mu_mailbox_get_url (mbox, &url);
  if (rc)
    return rc;
  mu_debug (MU_DEBCAT_MAILBOX, MU_DEBUG_TRACE1,
	    (_("opening mailbox %s"), mu_url_to_string (url)));
  rc = mu_url_sget_path (url, &mbox_name);
  if (rc == MU_ERR_NOENT)
    mbox_name = "INBOX";
  else if (rc)
    return rc;
      
  rc = mu_folder_open (folder, flags);
  if (rc)
    return rc;

  imap = folder->data;

  mu_imap_register_callback_function (imap, MU_IMAP_CB_RECENT_COUNT,
				      _imap_update_callback,
				      imbx);
  mu_imap_register_callback_function (imap, MU_IMAP_CB_MESSAGE_COUNT,
				      _imap_update_callback,
				      imbx);
  
  rc = mu_imap_select (imap, mbox_name,
		       flags & (MU_STREAM_WRITE|MU_STREAM_APPEND),
		       &imbx->stats);
  if (rc)
    return rc;

  if (imbx->stats.flags & MU_IMAP_STAT_MESSAGE_COUNT)
    rc = _imap_realloc_messages (imbx, imbx->stats.message_count);
  
  return rc;
}

static int
_imap_mbx_close (mu_mailbox_t mbox)
{
  int rc;
  mu_folder_t folder = mbox->folder;
  mu_imap_t imap = folder->data;

  mu_debug (MU_DEBCAT_MAILBOX, MU_DEBUG_TRACE1,
	    (_("closing mailbox %s"), mu_url_to_string (mbox->url)));
  if (mu_imap_capability_test (imap, "UNSELECT", NULL) == 0)
    rc = mu_imap_unselect (imap);
  else
    rc = mu_imap_close (imap);
  return rc;
}

static int
_imap_messages_count (mu_mailbox_t mbox, size_t *pcount)
{
  struct _mu_imap_mailbox *imbx = mbox->data;
  if (imbx->stats.flags & MU_IMAP_STAT_MESSAGE_COUNT)
    *pcount = imbx->stats.message_count;
  else
    return MU_ERR_INFO_UNAVAILABLE;
  return 0;
}
  
static int
_imap_messages_recent (mu_mailbox_t mbox, size_t *pcount)
{
  struct _mu_imap_mailbox *imbx = mbox->data;
  if (imbx->stats.flags & MU_IMAP_STAT_RECENT_COUNT)
    *pcount = imbx->stats.recent_count;
  else
    return MU_ERR_INFO_UNAVAILABLE;
  return 0;
}

static int
_imap_uidnext (mu_mailbox_t mbox, size_t *pn)
{
  struct _mu_imap_mailbox *imbx = mbox->data;
  if (imbx->stats.flags & MU_IMAP_STAT_UIDNEXT)
    *pn = imbx->stats.uidnext;
  else
    return MU_ERR_INFO_UNAVAILABLE;
  return 0;
}

static int
_imap_message_unseen (mu_mailbox_t mbox, size_t *pn)
{
  struct _mu_imap_mailbox *imbx = mbox->data;
  if (imbx->stats.flags & MU_IMAP_STAT_FIRST_UNSEEN)
    *pn = imbx->stats.uidnext;
  else
    return MU_ERR_INFO_UNAVAILABLE;
  return 0;
}

static int
_imap_uidvalidity (mu_mailbox_t mbox, unsigned long *pn)
{
  struct _mu_imap_mailbox *imbx = mbox->data;
  if (imbx->stats.flags & MU_IMAP_STAT_UIDVALIDITY)
    *pn = imbx->stats.uidvalidity;
  else
    return MU_ERR_INFO_UNAVAILABLE;
  return 0;
}

struct attr_tab
{
  size_t start;
  size_t end;
  int attr_flags;
};

static int
attr_tab_cmp (void const *a, void const *b)
{
  struct attr_tab const *ta = a;
  struct attr_tab const *tb = b;

  if (ta->attr_flags < tb->attr_flags)
    return -1;
  else if (ta->attr_flags > tb->attr_flags)
    return 1;

  if (ta->start < tb->start)
    return -1;
  else if (ta->start > tb->start)
    return 1;
  return 0;
} 

static int
aggregate_attributes (struct _mu_imap_mailbox *imbx,
		      struct attr_tab **ptab, size_t *pcnt)
{
  size_t i, j;
  size_t count;
  struct attr_tab *tab;
  
  /* Pass 1: Count modified attributes */
  count = 0;
  for (i = 0; i < imbx->msgs_cnt; i++)
    {
      if (imbx->msgs[i].flags & _MU_IMAP_MSG_ATTRCHG)
	count++;
    }

  if (count == 0)
    {
      *ptab = NULL;
      *pcnt = 0;
      return 0;
    }
  
  /* Pass 2: Create and populate expanded array */
  tab = calloc (count, sizeof (*tab));
  if (!tab)
    return ENOMEM;
  for (i = j = 0; i < imbx->msgs_cnt; i++)
    {
      if (imbx->msgs[i].flags & _MU_IMAP_MSG_ATTRCHG)
	{
	  tab[j].start = tab[j].end = i;
	  tab[j].attr_flags = imbx->msgs[i].attr_flags;
	  j++;
	}
    }
  
  /* Sort the array */
  qsort (tab, count, sizeof (tab[0]), attr_tab_cmp);

  /* Pass 3: Coalesce message ranges */
  for (i = j = 0; i < count; i++)
    {
      if (i == j)
	continue;
      else if ((tab[i].attr_flags == tab[j].attr_flags) &&
	       (tab[i].start == tab[j].end + 1))
	tab[j].end++;
      else
	tab[++j] = tab[i];
    }

  *ptab = tab;
  *pcnt = j + 1;
  return 0;
}

static int
_imap_mbx_gensync (mu_mailbox_t mbox, int *pdel)
{
  struct _mu_imap_mailbox *imbx = mbox->data;
  mu_folder_t folder = mbox->folder;
  mu_imap_t imap = folder->data;
  size_t i, j;
  mu_msgset_t msgset;
  int rc;
  int delflg = 0;
  struct attr_tab *tab;
  size_t count;

  rc = mu_msgset_create (&msgset, NULL, MU_MSGSET_NUM);
  if (rc)
    return rc;

  rc = aggregate_attributes (imbx, &tab, &count);
  if (rc)
    {
      /* Too bad, but try to use naive approach */
      for (i = 0; i < imbx->msgs_cnt; i++)
	{
	  if (imbx->msgs[i].flags & _MU_IMAP_MSG_ATTRCHG)
	    {
	      mu_msgset_clear (msgset);
	      mu_msgset_add_range (msgset, i + 1, i + 1, MU_MSGSET_NUM);
	      if (rc)
		break;
	      rc = mu_imap_store_flags (imap, 0, msgset,
					MU_IMAP_STORE_SET|MU_IMAP_STORE_SILENT,
					imbx->msgs[i].attr_flags);
	      delflg |= imbx->msgs[i].attr_flags & MU_ATTRIBUTE_DELETED;
	      if (rc)
		break;
	    }
	}
    }
  else
    {
      for (i = j = 0; i < count; i++)
	{
	  if (j < i)
	    {
	      if (tab[j].attr_flags != tab[i].attr_flags)
		{
		  rc = mu_imap_store_flags (imap, 0, msgset,
					    MU_IMAP_STORE_SET|
					    MU_IMAP_STORE_SILENT,
					    tab[j].attr_flags);
		  delflg |= tab[j].attr_flags & MU_ATTRIBUTE_DELETED;
		  if (rc)
		    break;
		  mu_msgset_clear (msgset);
		  j = i;
		}
	    }
	  if (tab[i].end == tab[i].start)
	    rc = mu_msgset_add_range (msgset,
				      tab[i].start + 1, tab[i].start + 1,
				      MU_MSGSET_NUM);
	  else
	    rc = mu_msgset_add_range (msgset,
				      tab[i].start + 1, tab[i].end + 1,
				      MU_MSGSET_NUM);
	  if (rc)
	    break;
	}

      if (rc == 0 && j < i)
	{
	  rc = mu_imap_store_flags (imap, 0, msgset,
				    MU_IMAP_STORE_SET|MU_IMAP_STORE_SILENT,
				    tab[j].attr_flags);
	  delflg |= tab[j].attr_flags & MU_ATTRIBUTE_DELETED;
	}
      free (tab);
    }
  mu_msgset_free (msgset);
  
  if (rc)
    return rc;

  if (pdel)
    *pdel = delflg;
  
  return 0;
}

static int
_imap_mbx_expunge (mu_mailbox_t mbox)
{
  int rc, del = 0;
  
  mu_debug (MU_DEBCAT_MAILBOX, MU_DEBUG_TRACE1,
	    (_("expunging mailbox %s"), mu_url_to_string (mbox->url)));
  rc = _imap_mbx_gensync (mbox, &del);
  if (rc == 0 && del)
    {
      mu_folder_t folder = mbox->folder;
      mu_imap_t imap = folder->data;
      rc = mu_imap_expunge (imap);
    }
  return rc;
}

static int
_imap_mbx_sync (mu_mailbox_t mbox)
{
  mu_debug (MU_DEBCAT_MAILBOX, MU_DEBUG_TRACE1,
	    (_("synchronizing mailbox %s"), mu_url_to_string (mbox->url)));
  return _imap_mbx_gensync (mbox, NULL);
}

static int
_imap_mbx_append_message (mu_mailbox_t mbox, mu_message_t msg)
{
  int rc;
  mu_folder_t folder = mbox->folder;
  mu_imap_t imap = folder->data;
  mu_url_t url;
  const char *mbox_name;
  
  rc = mu_mailbox_get_url (mbox, &url);
  if (rc)
    return rc;
  rc = mu_url_sget_path (url, &mbox_name);
  return mu_imap_append_message (imap, mbox_name, 0, NULL, NULL, msg);
}

static int _compute_lines (struct mu_bodystructure *bs, size_t *pcount);

static int
sum_lines (void *item, void *data)
{
  struct mu_bodystructure *bs = item;
  size_t *pn = data;
  size_t n;

  if (_compute_lines (bs, &n))
    return 1;
  *pn += n;
  return 0;
}

static int
_compute_lines (struct mu_bodystructure *bs, size_t *pcount)
{
  switch (bs->body_message_type)
    {
    case mu_message_other:
      break;

    case mu_message_text:
      *pcount = bs->v.text.body_lines;
      return 0;

    case mu_message_rfc822:
      *pcount = bs->v.rfc822.body_lines;
      return 0;
      
    case mu_message_multipart:
      *pcount = 0;
      return mu_list_foreach (bs->v.multipart.body_parts, sum_lines, pcount);
    }
  return 1;
}

static int
fetch_response_parser (void *item, void *data)
{
  union mu_imap_fetch_response *resp = item;
  struct _mu_imap_message *imsg = data;

  switch (resp->type)
    {
    case MU_IMAP_FETCH_UID:
      imsg->uid = resp->uid.uid;
      break;
      
    case MU_IMAP_FETCH_FLAGS:
      imsg->attr_flags = resp->flags.flags;
      break;
      
    case MU_IMAP_FETCH_ENVELOPE:
      imsg->env = resp->envelope.imapenvelope;
      resp->envelope.imapenvelope = NULL; /* Steal the envelope */
      break;
      
    case MU_IMAP_FETCH_RFC822_SIZE:
      imsg->message_size = resp->rfc822_size.size;
      break;
      
    case MU_IMAP_FETCH_BODYSTRUCTURE:
      {
	size_t n;
	if (_compute_lines (resp->bodystructure.bs, &n) == 0)
	  {
	    imsg->message_lines = n;
	    imsg->flags |= _MU_IMAP_MSG_LINES;
	  }
      }
      break;
      
    default:
      mu_debug (MU_DEBCAT_MAILBOX, MU_DEBUG_TRACE0,
		(_("fetch returned a not requested item %d"),
		 resp->type));
      break;
    }
  return 0;
}
  
static void
_imap_fetch_callback (void *data, int code, size_t sdat, void *pdat)
{
  struct _mu_imap_mailbox *imbx = data;
  mu_mailbox_t mbox = imbx->mbox;
  mu_list_t list = pdat;
  int rc;
  struct _mu_imap_message *imsg;
  
  rc = _imap_realloc_messages (imbx, sdat - 1);
  if (rc)
    {
      mu_debug (MU_DEBCAT_MAILBOX, MU_DEBUG_ERROR,
		(_("cannot reallocate array of messages: %s"),
		 mu_strerror (rc)));
      imbx->last_error = rc;
      return;
    }
  if (imbx->msgs_cnt < sdat)
    imbx->msgs_cnt = sdat;
  imsg = imbx->msgs + sdat - 1;
  imsg->imbx = imbx;
  mu_list_foreach (list, fetch_response_parser, imsg);

  if (mbox->observable)
    {
      if (((sdat + 1) % 10) == 0)
	mu_observable_notify (imbx->mbox->observable,
			      MU_EVT_MAILBOX_PROGRESS, NULL);
    }
}

static int
_imap_mbx_scan (mu_mailbox_t mbox, size_t msgno, size_t *pcount)
{
  struct _mu_imap_mailbox *imbx = mbox->data;
  mu_folder_t folder = mbox->folder;
  mu_imap_t imap = folder->data;
  mu_msgset_t msgset;
  int rc;
  static char _imap_scan_items[] = "(UID FLAGS ENVELOPE RFC822.SIZE BODY)";
  
  mu_debug (MU_DEBCAT_MAILBOX, MU_DEBUG_TRACE1,
	    (_("scanning mailbox %s"), mu_url_to_string (mbox->url)));
  rc = mu_msgset_create (&msgset, NULL, MU_MSGSET_NUM);
  if (rc)
    return rc;
  rc = mu_msgset_add_range (msgset, msgno, MU_MSGNO_LAST, MU_MSGSET_NUM);
  if (rc)
    {
      mu_msgset_free (msgset);
      return rc;
    }
  
  _imap_mbx_clrerr (imbx);
  rc = _imap_fetch_with_callback (imap, msgset, _imap_scan_items,
				  _imap_fetch_callback, imbx);
  mu_msgset_free (msgset);
  if (rc == 0)
    rc = _imap_mbx_errno (imbx);
  if (rc == 0)
    {
      size_t i;
      mu_off_t total = 0;

      imbx->flags |= _MU_IMAP_MBX_UPTODATE;

      for (i = 1; i <= imbx->msgs_cnt; i++)
	{
	  total += imbx->msgs[i-1].message_size;
	  /* MU_EVT_MESSAGE_ADD must be delivered only when it is already
	     possible to retrieve the message in question.  It could not be
	     done in the fetch handler for obvious reasons.  Hence the extra
	     loop. */
	  if (mbox->observable)
	    mu_observable_notify (mbox->observable, MU_EVT_MESSAGE_ADD, &i);
	}
      imbx->total_size = total;
      
      if (pcount)
	*pcount = imbx->msgs_cnt;
    }
  return rc;
}

static int
_imap_mbx_is_updated (mu_mailbox_t mbox)
{
  struct _mu_imap_mailbox *imbx = mbox->data;
  mu_folder_t folder = mbox->folder;
  mu_imap_t imap = folder->data;
  int rc;
  
  rc = mu_imap_noop (imap);
  if (rc)
    {
      mu_debug (MU_DEBCAT_MAILBOX, MU_DEBUG_ERROR,
		(_("mu_imap_noop: %s"), mu_strerror (rc)));
      imbx->last_error = rc;
    }
  return imbx->flags & _MU_IMAP_MBX_UPTODATE;
}

static int
_imap_copy_to_mailbox (mu_mailbox_t mbox, mu_msgset_t msgset,
		       const char *mailbox, int flags)
{
  struct _mu_imap_mailbox *imbx = mbox->data;
  mu_folder_t folder = mbox->folder;
  mu_imap_t imap = folder->data;
  int rc;
  
  mu_debug (MU_DEBCAT_MAILBOX, MU_DEBUG_TRACE1,
	    (_("copying messages to mailbox %s"), mailbox));
  _imap_mbx_clrerr (imbx);

  rc = mu_imap_copy (imap, flags & MU_MAILBOX_COPY_UID, msgset, mailbox);
  if (rc)
    {
      mu_debug (MU_DEBCAT_MAILBOX, MU_DEBUG_ERROR,
		(_("mu_imap_copy: %s"), mu_strerror (rc)));
      if (rc)
	{
	  if (mu_imap_response_code (imap) == MU_IMAP_RESPONSE_TRYCREATE)
	    {
	      mu_debug (MU_DEBCAT_MAILBOX, MU_DEBUG_TRACE1,
			(_("creating mailbox %s"), mailbox));
	      rc = mu_imap_mailbox_create (imap, mailbox);
	      if (rc == 0)
		rc = mu_imap_copy (imap, flags & MU_MAILBOX_COPY_UID,
				   msgset, mailbox);
	    }
	}
      imbx->last_error = rc;
    }
  return rc;
}

int
_mu_imap_mailbox_init (mu_mailbox_t mailbox)
{
  struct _mu_imap_mailbox *mbx = calloc (1, sizeof (*mbx));

  if (!mbx)
    return ENOMEM;
  mbx->mbox = mailbox;
  mailbox->data = mbx;

  mailbox->_destroy = _imap_mbx_destroy;
  mailbox->_open = _imap_mbx_open;
  mailbox->_close = _imap_mbx_close;
  mailbox->_expunge = _imap_mbx_expunge;

  mailbox->_messages_count = _imap_messages_count;
  mailbox->_messages_recent = _imap_messages_recent;
  mailbox->_message_unseen = _imap_message_unseen;
  mailbox->_uidvalidity = _imap_uidvalidity;
  mailbox->_uidnext = _imap_uidnext;
  
  mailbox->_scan = _imap_mbx_scan;
  mailbox->_is_updated = _imap_mbx_is_updated;
  mailbox->_get_message = _imap_mbx_get_message;
  mailbox->_sync = _imap_mbx_sync;
  
  mailbox->_append_message = _imap_mbx_append_message;
  mailbox->_copy = _imap_copy_to_mailbox;
  
  return 0;
}
