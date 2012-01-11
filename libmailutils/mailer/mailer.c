/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2001, 2004-2007, 2009-2012 Free Software
   Foundation, Inc.

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
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>

#include <sys/time.h>

#include <mailutils/nls.h>
#include <mailutils/cstr.h>
#include <mailutils/address.h>
#include <mailutils/debug.h>
#include <mailutils/sys/debcat.h>
#include <mailutils/errno.h>
#include <mailutils/iterator.h>
#include <mailutils/list.h>
#include <mailutils/observer.h>
#include <mailutils/property.h>
#include <mailutils/registrar.h>
#include <mailutils/stream.h>
#include <mailutils/url.h>
#include <mailutils/header.h>
#include <mailutils/envelope.h>
#include <mailutils/body.h>
#include <mailutils/mailbox.h>
#include <mailutils/message.h>
#include <mailutils/wordsplit.h>
#include <mailutils/util.h>
#include <mailutils/mime.h>
#include <mailutils/io.h>
#include <mailutils/cctype.h>
#include <mailutils/parse822.h>

#include <mailutils/sys/mailer.h>

static char *mailer_url_default;

/* FIXME: I'd like to check that the URL is valid, but that requires that the
   mailers already be registered! */
int
mu_mailer_set_url_default (const char *url)
{
  char *n = NULL;

  if (!url)
    return EINVAL;

  if ((n = strdup (url)) == NULL)
    return ENOMEM;

  if (mailer_url_default)
    free (mailer_url_default);

  mailer_url_default = n;

  return 0;
}

int
mu_mailer_get_url_default (const char **url)
{
  if (!url)
    return EINVAL;

  if (mailer_url_default)
    *url = mailer_url_default;
  else
    *url = MAILER_URL_DEFAULT;

  return 0;
}

int
mu_mailer_create_from_url (mu_mailer_t *pmailer, mu_url_t url)
{
  mu_record_t record;

  if (mu_registrar_lookup_url (url, MU_FOLDER_ATTRIBUTE_FILE, &record,
			       NULL) == 0)
    {
      int (*m_init) (mu_mailer_t) = NULL;

      mu_record_get_mailer (record, &m_init);
      if (m_init)
        {
	  int status;
	  mu_mailer_t mailer;
	  int (*u_init) (mu_url_t) = NULL;
	  
	  /* Allocate memory for mailer.  */
	  mailer = calloc (1, sizeof (*mailer));
	  if (mailer == NULL)
	    return ENOMEM;

	  status = mu_monitor_create (&mailer->monitor, 0, mailer);
	  if (status)
	    {
	      mu_mailer_destroy (&mailer);
	      return status;
	    }

	  status = m_init (mailer);
	  if (status)
	    {
	      mu_mailer_destroy (&mailer);
	      return status;
	    }

	  mu_record_get_url (record, &u_init);
	  if (u_init && (status = u_init (url)) != 0)
	    {
	      mu_mailer_destroy (&mailer);
	      return status;
	    }
	  
	  mailer->url = url;
	  *pmailer = mailer;

	  return status;
	}
    }
  
    return MU_ERR_MAILER_BAD_URL;
}

int
mu_mailer_create (mu_mailer_t * pmailer, const char *name)
{
  int status;
  mu_url_t url;

  if (name == NULL)
    mu_mailer_get_url_default (&name);

  status = mu_url_create (&url, name);
  if (status)
    return status;
  status = mu_mailer_create_from_url (pmailer, url);
  if (status)
    mu_url_destroy (&url);
  return status;
}

void
mu_mailer_destroy (mu_mailer_t * pmailer)
{
  if (pmailer && *pmailer)
    {
      mu_mailer_t mailer = *pmailer;
      mu_monitor_t monitor = mailer->monitor;

      if (mailer->observable)
	{
	  mu_observable_notify (mailer->observable, MU_EVT_MAILER_DESTROY,
				mailer);
	  mu_observable_destroy (&mailer->observable, mailer);
	}

      /* Call the object destructor.  */
      if (mailer->_destroy)
	mailer->_destroy (mailer);

      mu_monitor_wrlock (monitor);

      if (mailer->stream)
	{
	  /* FIXME: Should be the client responsability to close this?  */
	  /* mu_stream_close (mailer->stream); */
	  mu_stream_destroy (&mailer->stream);
	}

      if (mailer->url)
	mu_url_destroy (&(mailer->url));

      if (mailer->property)
	mu_property_destroy (&mailer->property);

      free (mailer);
      *pmailer = NULL;
      mu_monitor_unlock (monitor);
      mu_monitor_destroy (&monitor, mailer);
    }
}


/* -------------- stub functions ------------------- */

int
mu_mailer_open (mu_mailer_t mailer, int flag)
{
  if (mailer == NULL || mailer->_open == NULL)
    return ENOSYS;
  return mailer->_open (mailer, flag);
}

int
mu_mailer_close (mu_mailer_t mailer)
{
  if (mailer == NULL || mailer->_close == NULL)
    return ENOSYS;
  return mailer->_close (mailer);
}


int
mu_mailer_check_from (mu_address_t from)
{
  size_t n = 0;

  if (!from)
    return EINVAL;

  if (mu_address_get_count (from, &n) || n != 1)
    return MU_ERR_MAILER_BAD_FROM;

  if (mu_address_get_email_count (from, &n) || n == 0)
    return MU_ERR_MAILER_BAD_FROM;

  return 0;
}

int
mu_mailer_check_to (mu_address_t to)
{
  size_t count = 0;
  size_t emails = 0;
  size_t groups = 0;

  if (!to)
    return EINVAL;

  if (mu_address_get_count (to, &count))
    return MU_ERR_MAILER_BAD_TO;

  if (mu_address_get_email_count (to, &emails))
    return MU_ERR_MAILER_BAD_TO;

  if (emails == 0)
    return MU_ERR_MAILER_NO_RCPT_TO;

  if (mu_address_get_group_count (to, &groups))
    return MU_ERR_MAILER_BAD_TO;

  if (count - emails - groups != 0)
    /* then not everything is a group or an email address */
    return MU_ERR_MAILER_BAD_TO;

  return 0;
}

static void
save_fcc (mu_message_t msg)
{
  mu_header_t hdr;
  size_t count = 0, i;
  const char *buf;
  char *fcc;
  
  if (mu_message_get_header (msg, &hdr))
    return;

  if (mu_header_get_value (hdr, MU_HEADER_FCC, NULL, 0, NULL))
    return;
  
  mu_header_get_field_count (hdr, &count);
  for (i = 1; i <= count; i++)
    {
      mu_mailbox_t mbox;
      
      mu_header_sget_field_name (hdr, i, &buf);
      if (mu_c_strcasecmp (buf, MU_HEADER_FCC) == 0
	  && mu_header_aget_field_value (hdr, i, &fcc) == 0)
	{
	  size_t i;
	  struct mu_wordsplit ws;

	  ws.ws_delim = ",";
	  if (mu_wordsplit (fcc, &ws,
			    MU_WRDSF_DEFFLAGS|MU_WRDSF_DELIM|MU_WRDSF_WS))
	    {
	      mu_error (_("cannot split line `%s': %s"), fcc,
			mu_wordsplit_strerror (&ws));
	      continue;
	    }

	  for (i = 0; i < ws.ws_wordc; i += 2)
	    {
	      if (mu_mailbox_create_default (&mbox, ws.ws_wordv[i]))
		continue; /*FIXME: error message?? */
	      if (mu_mailbox_open (mbox,
				   MU_STREAM_RDWR | MU_STREAM_CREAT
				   | MU_STREAM_APPEND) == 0)
		{
		  mu_mailbox_append_message (mbox, msg);
		  mu_mailbox_flush (mbox, 0);
		}
	      mu_mailbox_close (mbox);
	      mu_mailbox_destroy (&mbox);
	    }
	  mu_wordsplit_free (&ws);
	  free (fcc);
	}
    }
}

static int
copy_fragment (char **pretender, const char *p, const char *q)
{
  size_t len = q - p + 1;
  *pretender = malloc (len + 1);
  if (!*pretender)
    return ENOMEM;
  memcpy (*pretender, p, len);
  (*pretender)[len] = 0;
  return 0;
}

/* Try to extract from STRING a portion that looks like an email address */
static int
recover_email (const char *string, char **pretender)
{
  char *p, *q;

  p = strchr (string, '<');
  if (p)
    {
      q = strchr (p, '>');
      if (q)
	return copy_fragment (pretender, p, q);
    }
  p = mu_str_skip_class (string, MU_CTYPE_SPACE);
  if (*p && mu_parse822_is_atom_char (*p))
    {
      q = p;
      while (*++q && (mu_parse822_is_atom_char (*q) || *q == '.'))
	;
      if (*q == '@')
	while (*++q && (mu_parse822_is_atom_char (*q) || *q == '.'))
	  ;
      q--;
      if (q > p)
	return copy_fragment (pretender, p, q);
    }
  return MU_ERR_NOENT;
}

static int
safe_address_create (mu_address_t *paddr, const char *addr_str,
		     const char *who)
{
  int status = mu_address_create (paddr, addr_str);
  if (status == MU_ERR_INVALID_EMAIL)
    {
      int rc;
      char *p;
      
      mu_debug (MU_DEBCAT_MAILER, MU_DEBUG_ERROR,
		("bad %s address: %s", who, addr_str));
      rc = recover_email (addr_str, &p);
      if (rc)
	{
	  if (rc != MU_ERR_NOENT)
	    mu_debug (MU_DEBCAT_MAILER, MU_DEBUG_ERROR,
		      ("%s address recovery failed: %s",
		       who, mu_strerror (rc)));
	}
      else
	{
	  mu_debug (MU_DEBCAT_MAILER, MU_DEBUG_TRACE1,
		    ("recovered possible %s address: %s", who, p));
	  rc = mu_address_create (paddr, p);
	  if (rc == 0)
	    status = 0;
	  else if (rc == MU_ERR_INVALID_EMAIL)
	    mu_debug (MU_DEBCAT_MAILER, MU_DEBUG_TRACE1,
		      ("%s address guess failed", who));
	  else
	    mu_debug (MU_DEBCAT_MAILER, MU_DEBUG_ERROR,
		      ("cannot convert %s address '%s': %s",
		       who, p, mu_strerror (rc)));
	  free (p);
	}
    }
  return status;
}

static int
_set_from (mu_address_t *pfrom, mu_message_t msg, mu_address_t from,
	   mu_mailer_t mailer)
{
  int status = 0;

  *pfrom = NULL;
  
  /* Get MAIL_FROM from URL, envelope, headers, or the environment. */
  if (!from)
    {
      const char *type;
      mu_envelope_t env;
      const char *mail_from;

      status = mu_url_sget_param (mailer->url, "from", &mail_from);

      if (status)
	{
	  status = mu_message_get_envelope (msg, &env);
	  if (status)
	    return status;

	  status = mu_envelope_sget_sender (env, &mail_from);
	  if (status)
	    {
	      mu_header_t header;
	      status = mu_message_get_header (msg, &header);
	      if (status)
		return status;
	      status = mu_header_sget_value (header, MU_HEADER_FROM,
					     &mail_from);
	    }
	}
      
      switch (status)
	{
	default:
	  return status;

	  /* Use the From: header. */
	case 0:
	  mu_debug (MU_DEBCAT_MAILER, MU_DEBUG_TRACE,
		     ("mu_mailer_send_message(): using From: %s",
		      mail_from));
	  break;

	case MU_ERR_NOENT:
	  if (mu_property_sget_value (mailer->property, "TYPE", &type) == 0
	      && strcmp (type, "SENDMAIL") == 0)
	    return 0;
	  
	  /* Use the environment. */
	  mail_from = mu_get_user_email (NULL);

	  if (mail_from)
            mu_debug (MU_DEBCAT_MAILER, MU_DEBUG_TRACE,
		       ("mu_mailer_send_message(): using user's address: %s",
			mail_from));
	  else
	    {
	      mu_debug (MU_DEBCAT_MAILER, MU_DEBUG_ERROR,
			("mu_mailer_send_message(): "
			 "no user's address, failing"));
	      return errno;
	    }
	  /* FIXME: should we add the From: header? */
	  break;
	}
      status = safe_address_create (pfrom, mail_from, "sender");
    }
  
  return status;
}

static int
_set_to (mu_address_t *paddr, mu_message_t msg, mu_address_t to,
	 mu_mailer_t mailer)
{
  int status = 0;

  *paddr = NULL;
  if (!to)
    {
      const char *rcpt_to;

      status = mu_url_sget_param (mailer->url, "to", &rcpt_to);
      switch (status)
	{
	case 0:
	  break;

	case MU_ERR_NOENT:
	  /* FIXME: Get it from the message itself, at least if the
	     mailer is not SENDMAIL. */
	  return 0;

	default:
	  return status;
	}
      mu_debug (MU_DEBCAT_MAILER, MU_DEBUG_TRACE,
		("mu_mailer_send_message(): using RCPT TO: %s",
		 rcpt_to));
      status = safe_address_create (paddr, rcpt_to, "recipient");
    }
  
  return status;
}
  

  
static int
create_part (mu_mime_t mime, mu_stream_t istr, 
	     size_t fragsize, size_t n, size_t nparts, char *msgid)
{
  int status = 0;
  mu_message_t newmsg;
  mu_header_t newhdr;
  mu_body_t body;
  mu_stream_t ostr;
  char buffer[512], *str;
  size_t slen;
  
  mu_message_create (&newmsg, NULL);
  mu_message_get_header (newmsg, &newhdr); 

  str = NULL;
  slen = 0;
  mu_asnprintf (&str, &slen,
		"message/partial; id=\"%s\"; number=%lu; total=%lu",
		msgid, (unsigned long)n, (unsigned long)nparts);
  mu_header_append (newhdr, MU_HEADER_CONTENT_TYPE, str);
  mu_asnprintf (&str, &slen, "part %lu of %lu",
		(unsigned long)n, (unsigned long)nparts);
  mu_header_append (newhdr, MU_HEADER_CONTENT_DESCRIPTION, str);
  free (str);
  
  mu_message_get_body (newmsg, &body);
  mu_body_get_streamref (body, &ostr);

  status = mu_stream_seek (ostr, 0, SEEK_SET, NULL);
  if (status == 0)
    while (fragsize)
      {
	size_t rds = fragsize;
	if (rds > sizeof buffer)
	  rds = sizeof buffer;
	
	status = mu_stream_read (istr, buffer, rds, &rds);
	if (status || rds == 0)
	  break;
	status = mu_stream_write (ostr, buffer, rds, NULL);
	if (status)
	  break;
	fragsize -= rds;
      }
  mu_stream_destroy (&ostr);
  
  if (status == 0)
    {
      mu_mime_add_part (mime, newmsg);
      mu_message_unref (newmsg);
    }
  return status;
}

static void
merge_headers (mu_message_t newmsg, mu_header_t hdr)
{
  size_t i, count;
  mu_header_t newhdr;
  
  mu_message_get_header (newmsg, &newhdr);
  mu_header_get_field_count (hdr, &count);
  for (i = 1; i <= count; i++)
    {
      const char *fn, *fv;

      mu_header_sget_field_name (hdr, i, &fn);
      mu_header_sget_field_value (hdr, i, &fv);
      if (mu_c_strcasecmp (fn, MU_HEADER_MESSAGE_ID) == 0)
	continue;
      else if (mu_c_strcasecmp (fn, MU_HEADER_MIME_VERSION) == 0)
	mu_header_append (newhdr, "X-Orig-" MU_HEADER_MIME_VERSION,
			  fv);
      else if (mu_c_strcasecmp (fn, MU_HEADER_CONTENT_TYPE) == 0)
	mu_header_append (newhdr, "X-Orig-" MU_HEADER_CONTENT_TYPE,
			  fv);
      else if (mu_c_strcasecmp (fn, MU_HEADER_CONTENT_DESCRIPTION) == 0)
	mu_header_append (newhdr, "X-Orig-" MU_HEADER_CONTENT_DESCRIPTION,
			  fv);
      else
	mu_header_append (newhdr, fn, fv);
    }
}
  

int
send_fragments (mu_mailer_t mailer,
		mu_header_t hdr,
		mu_stream_t str,
		size_t nparts, size_t fragsize,
		struct timeval *delay,
		mu_address_t from, mu_address_t to)
{
  int status = 0;
  size_t i;
  char *msgid = NULL;
  
  if (mu_header_aget_value (hdr, MU_HEADER_MESSAGE_ID, &msgid))
    mu_rfc2822_msg_id (0, &msgid);
  
  for (i = 1; i <= nparts; i++)
    {
      mu_message_t newmsg;
      mu_mime_t mime;
		  
      mu_mime_create (&mime, NULL, 0);
      status = create_part (mime, str, fragsize, i, nparts, msgid);
      if (status)
	break;

      mu_mime_to_message (mime, &newmsg);
      merge_headers (newmsg, hdr);
      
      status = mailer->_send_message (mailer, newmsg, from, to);
      mu_message_unref (newmsg);
      if (status)
	break;
      if (delay)
	{
	  struct timeval t = *delay;
	  select (0, NULL, NULL, NULL, &t);
	}
    }
  free (msgid);
  return status;
}

int
mu_mailer_send_fragments (mu_mailer_t mailer,
			  mu_message_t msg,
			  size_t fragsize, struct timeval *delay,
			  mu_address_t from, mu_address_t to)
{
  int status;
  mu_address_t sender_addr = NULL, rcpt_addr = NULL;
  
  if (mailer == NULL)
    return EINVAL;
  if (mailer->_send_message == NULL)
    return ENOSYS;

  status = _set_from (&sender_addr, msg, from, mailer);
  if (status)
    return status;
  if (sender_addr)
    from = sender_addr;

  status = _set_to (&rcpt_addr, msg, to, mailer);
  if (status)
    return status;
  if (rcpt_addr)
    to = rcpt_addr;
  
  if ((!from || (status = mu_mailer_check_from (from)) == 0)
      && (!to || (status = mu_mailer_check_to (to)) == 0))
    {
      save_fcc (msg);
      if (fragsize == 0)
	status = mailer->_send_message (mailer, msg, from, to);
      else
	{
	  mu_header_t hdr;
	  mu_body_t body;
	  size_t bsize;
	  size_t nparts;
	  
	  /* Estimate the number of messages to be sent. */
	  mu_message_get_header (msg, &hdr);

	  mu_message_get_body (msg, &body);
	  mu_body_size (body, &bsize);

	  nparts = bsize + fragsize - 1;
	  if (nparts < bsize) /* overflow */
	    return EINVAL;
	  nparts /= fragsize;

	  if (nparts == 1)
	    status = mailer->_send_message (mailer, msg, from, to);
	  else
	    {
	      mu_stream_t str;
	      status = mu_body_get_streamref (body, &str);
	      if (status)
		{
		  status = send_fragments (mailer, hdr, str, nparts,
					   fragsize, delay, from, to);
		  mu_stream_destroy (&str);
		}
	    }
	}
    }
  mu_address_destroy (&sender_addr);
  mu_address_destroy (&rcpt_addr);
  return status;
}

int
mu_mailer_send_message (mu_mailer_t mailer, mu_message_t msg,
			mu_address_t from, mu_address_t to)
{
  return mu_mailer_send_fragments (mailer, msg, 0, NULL, from, to);
}

int
mu_mailer_set_stream (mu_mailer_t mailer, mu_stream_t stream)
{
  if (mailer == NULL)
    return EINVAL;
  mailer->stream = stream;
  return 0;
}

int
mu_mailer_get_stream (mu_mailer_t mailer, mu_stream_t * pstream)
{
  /* FIXME: Deprecation warning */
  if (mailer == NULL)
    return EINVAL;
  if (pstream == NULL)
    return MU_ERR_OUT_PTR_NULL;
  *pstream = mailer->stream;
  return 0;
}

int
mu_mailer_get_streamref (mu_mailer_t mailer, mu_stream_t * pstream)
{
  if (mailer == NULL)
    return EINVAL;
  if (pstream == NULL)
    return MU_ERR_OUT_PTR_NULL;
  return mu_streamref_create (pstream, mailer->stream);
}

int
mu_mailer_get_observable (mu_mailer_t mailer, mu_observable_t * pobservable)
{
  /* FIXME: I should check for invalid types */
  if (mailer == NULL)
    return EINVAL;
  if (pobservable == NULL)
    return MU_ERR_OUT_PTR_NULL;
  if (mailer->observable == NULL)
    {
      int status = mu_observable_create (&(mailer->observable), mailer);
      if (status != 0)
	return status;
    }
  *pobservable = mailer->observable;
  return 0;
}

int
mu_mailer_set_property (mu_mailer_t mailer, mu_property_t property)
{
  if (mailer == NULL)
    return EINVAL;
  if (mailer->property)
    mu_property_unref (mailer->property);
  mailer->property = property;
  mu_property_ref (property);
  return 0;
}

int
mu_mailer_get_property (mu_mailer_t mailer, mu_property_t * pproperty)
{
  if (mailer == NULL)
    return EINVAL;
  if (pproperty == NULL)
    return MU_ERR_OUT_PTR_NULL;
  if (mailer->property == NULL)
    {
      int status;

      if (mailer->_get_property)
	status = mailer->_get_property (mailer, &mailer->property);
      else
	status = mu_property_create_init (&mailer->property,
					  mu_assoc_property_init, NULL);
      if (status != 0)
	return status;
    }
  *pproperty = mailer->property;
  return 0;
}

int
mu_mailer_get_url (mu_mailer_t mailer, mu_url_t * purl)
{
  if (!mailer)
    return EINVAL;
  if (!purl)
    return MU_ERR_OUT_PTR_NULL;
  *purl = mailer->url;
  return 0;
}
