/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2007, 2009-2012 Free Software Foundation, Inc.

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

#include <stdlib.h>
#include <string.h>
#include <mailutils/address.h>
#include <mailutils/debug.h>
#include <mailutils/errno.h>
#include <mailutils/error.h>
#include <mailutils/property.h>
#include <mailutils/mailer.h>
#include <mailutils/url.h>
#include <mailutils/util.h>
#include <mailutils/wordsplit.h>
#include <mailutils/message.h>
#include <mailutils/envelope.h>
#include <mailutils/header.h>
#include <mailutils/sys/mailbox.h>
#include <mailutils/sys/mailer.h>

struct remote_mbox_data
{
  mu_mailer_t mailer;
};

static void
remote_mbox_destroy (mu_mailbox_t mailbox)
{
  if (mailbox->data)
    {
      struct remote_mbox_data *dat = mailbox->data;
      mu_mailer_destroy (&dat->mailer);
      free (dat);
      mailbox->data = NULL;
    }
}

static int
remote_mbox_open (mu_mailbox_t mbox, int flags)
{
  struct remote_mbox_data *dat = mbox->data;
  int status;
  int mflags = 0;
  
  if (!dat->mailer)
    return EINVAL;

  if (mu_debug_level_p (MU_DEBCAT_MAILBOX, MU_DEBUG_TRACE7))
    mflags = MAILER_FLAG_DEBUG_DATA;
  status = mu_mailer_open (dat->mailer, mflags);
  if (status)
    {
      mu_debug (MU_DEBCAT_MAILBOX, MU_DEBUG_ERROR,
		("cannot open mailer: %s", mu_strerror (status)));
      return status;
    }
  mbox->flags = flags;
  return 0;
}

static int
remote_mbox_close (mu_mailbox_t mbox)
{
  struct remote_mbox_data *dat = mbox->data;
  int status;
  
  mu_debug (MU_DEBCAT_MAILBOX, MU_DEBUG_TRACE1, ("remote_mbox_close"));
  status = mu_mailer_close (dat->mailer);
  if (status)
    mu_debug (MU_DEBCAT_MAILBOX, MU_DEBUG_ERROR, ("closing mailer failed: %s",
	       mu_strerror (status)));
  return status;
}

static int
mkaddr (mu_mailbox_t mbox, mu_property_t property,
	const char *key, mu_address_t *addr)
{
  const char *str = NULL;
  mu_property_sget_value (property, key, &str);
  if (str && *str)
    {
      int status = mu_address_create (addr, str);
      if (status)
	{
	  mu_debug (MU_DEBCAT_MAILBOX, MU_DEBUG_ERROR,
		    ("%s: %s mu_address_create failed: %s",
		     str, key, mu_strerror (status)));
	  return status;
	}
    }
  else
    *addr = NULL;
  return 0;
}

static int
parse_received (mu_header_t hdr, char **sptr)
{
  const char *recv;
  size_t i;
  struct mu_wordsplit ws;
  enum { rcv_init, rcv_from, rcv_by, rcv_for } state;
  int status;
  char *s;
  size_t len;
  
  status = mu_header_sget_value (hdr, MU_HEADER_RECEIVED, &recv);
  if (status)
    return status;
  if (mu_wordsplit (recv, &ws, MU_WRDSF_DEFFLAGS))
    return status;

  state = rcv_init;
  for (i = 0; i < ws.ws_wordc && state != rcv_for; i++)
    {
      switch (state)
	{
	case rcv_init:
	  if (strcmp (ws.ws_wordv[i], "from") == 0)
	    state = rcv_from;
	  break;
	  
	case rcv_from:
	  if (strcmp (ws.ws_wordv[i], "by") == 0)
	    state = rcv_by;
	  break;
	  
	case rcv_by:
	  if (strcmp (ws.ws_wordv[i], "for") == 0)
	    state = rcv_for;
	  break;

	default:
	  break;
	}
    }

  if (state != rcv_for || ws.ws_wordv[i] == NULL)
    return MU_ERR_NOENT;

  s = ws.ws_wordv[i];
  len = strlen (s);
  if (s[len - 1] == ';')
    len--;
  if (s[0] == '<' && s[len - 1] == '>')
    {
      s++;
      len--;
    }
  *sptr = malloc (len);
  if (!*sptr)
    status = ENOMEM;
  else
    {
      memcpy (*sptr, s, len);
      (*sptr)[len - 1] = 0;
    }
  mu_wordsplit_free (&ws);
  return status;
} 
  
static int
guess_message_recipient (mu_message_t msg, char **hdrname, char **pptr)
{
  mu_header_t hdr;
  int status;
  char *s = NULL;
  
  status = mu_message_get_header (msg, &hdr);
  if (status)
    return status;

  /* First, try an easy way. */
  if (hdrname)
    {
      int i;
      for (i = 0; hdrname[i]; i++)
	{
	  status = mu_header_aget_value (hdr, hdrname[i], &s);
	  if (status == 0 && *s != 0)
	    break;
	}
    }
  else
    status = MU_ERR_NOENT;
  
  if (status == MU_ERR_NOENT)
    {
      status = parse_received (hdr, &s);
      if (status)
	status = mu_header_aget_value (hdr, MU_HEADER_TO, &s);
    }
  
  if (status)
    return status;
  
  *pptr = s;
  
  return 0;
}
  

static int
remote_mbox_append_message (mu_mailbox_t mbox, mu_message_t msg)
{
  struct remote_mbox_data *dat = mbox->data;
  int status;
  mu_property_t property = NULL;
  mu_address_t from, to;
  
  if (!dat->mailer)
    return EINVAL;

  status = mu_mailbox_get_property (mbox, &property);
  if (status)
    mu_debug (MU_DEBCAT_MAILBOX, MU_DEBUG_ERROR, 
              ("failed to get property: %s",
	        mu_strerror (status)));

  mkaddr (mbox, property, "FROM", &from);
  mkaddr (mbox, property, "TO", &to);
  if (!to)
    {
      char *rcpt;
      
      status = mu_url_aget_param (mbox->url, "to", &rcpt);
      if (status == MU_ERR_NOENT)
	{
	  static char *hdrnames[] = {
	    "X-Envelope-To",
	    "Delivered-To",
	    "X-Original-To",
	    NULL
	  };
	  const char *hstr;
	  int hc; 
	  char **hv;
	  struct mu_wordsplit ws;
	  
	  if (mu_url_sget_param (mbox->url, "recipient-headers", &hstr) == 0)
	    {
	      if (*hstr == 0)
		{
		  hc = 0;
		  hv = NULL;
		}
	      else
		{
		  ws.ws_delim = ",";
		  if (mu_wordsplit (hstr, &ws,
				    MU_WRDSF_DEFFLAGS|MU_WRDSF_DELIM|MU_WRDSF_WS))
		    return errno;
		  hc = 1;
		  hv = ws.ws_wordv;
		}
	    }
	  else
	    {
	      hc = 0;
	      hv = hdrnames;
	    }
	  
	  status = guess_message_recipient (msg, hv, &rcpt);
	  if (hc)
	    mu_wordsplit_free (&ws);
	}

      if (status != MU_ERR_NOENT)
	{
	  const char *host;
	  struct mu_address hint;
	  
	  if (status)
	    {
	      mu_debug (MU_DEBCAT_MAILBOX, MU_DEBUG_ERROR,
			("failed to get recipient: %s",
			 mu_strerror (status)));
	      return status;
	    }

	  /* Get additional parameters */
	  status = mu_url_sget_param (mbox->url, "strip-domain", NULL);
	  if (status == 0)
	    {
	      char *q = strchr (rcpt, '@');
	      if (q)
		*q = 0;
	    }

	  status = mu_url_sget_param (mbox->url, "domain", &host);
	  if (!(status == 0 && *host))
	    mu_url_sget_host (mbox->url, &host);
	  hint.domain = (char*) host;
	  status = mu_address_create_hint (&to, rcpt, &hint, 
	                                   MU_ADDR_HINT_DOMAIN);
	  free (rcpt);
	  if (status)
	    {
	      mu_debug (MU_DEBCAT_MAILBOX, MU_DEBUG_ERROR,
			("%s: %s mu_address_create failed: %s",
			 rcpt, "TO", mu_strerror (status)));
	      return status;
	    }
	}
    }
  
  status = mu_mailer_send_message (dat->mailer, msg, from, to);

  if (status)
    mu_debug (MU_DEBCAT_MAILBOX, MU_DEBUG_ERROR,
	      ("Sending message failed: %s", mu_strerror (status)));
  return status;
}

static int
remote_mbox_scan (mu_mailbox_t mbox, size_t offset, size_t *pcount)
{
  if (pcount)
    *pcount = 0;
  return 0;
}

static int
remote_get_size (mu_mailbox_t mbox, mu_off_t *psize)
{
  if (psize)
    *psize = 0;
  return 0;
}

static int
remote_sync (mu_mailbox_t mbox)
{
  return 0;
}

int
_mu_mailer_mailbox_init (mu_mailbox_t mailbox)
{
  struct remote_mbox_data *dat;
  int rc;
  mu_mailer_t mailer;
  mu_url_t url;
  
  if (mailbox == NULL)
    return EINVAL;

  mu_debug (MU_DEBCAT_MAILBOX, MU_DEBUG_TRACE1,
	    ("_mu_mailer_mailbox_init(%s)",
	     mu_url_to_string (mailbox->url)));

  rc = mu_url_dup (mailbox->url, &url);
  if (rc)
    return rc;

  rc = mu_mailer_create_from_url (&mailer, url);
  if (rc)
    {
      mu_debug (MU_DEBCAT_MAILBOX, MU_DEBUG_ERROR,
		("_mu_mailer_mailbox_init(%s): cannot create mailer: %s",
		 mu_url_to_string (url), mu_strerror (rc)));
      mu_url_destroy (&url);
      return rc;
    }
  
  dat = mailbox->data = calloc (1, sizeof (*dat));
  if (dat == NULL)
    {
      mu_mailer_destroy (&mailer);
      return ENOMEM;
    }
  dat->mailer = mailer;

  mailbox->_destroy = remote_mbox_destroy;
  mailbox->_open = remote_mbox_open;
  mailbox->_close = remote_mbox_close;
  mailbox->_append_message = remote_mbox_append_message;
  mailbox->_scan = remote_mbox_scan;
  mailbox->_get_size = remote_get_size;
  mailbox->_sync = remote_sync;

  return 0;
}

int
_mu_mailer_folder_init (mu_folder_t folder MU_ARG_UNUSED)
{
  return 0;
}
