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
# include <config.h>
#endif

#ifdef ENABLE_SMTP

#include <errno.h>
#include <netdb.h>
#include <pwd.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>

#include <mailutils/nls.h>
#include <mailutils/address.h>
#include <mailutils/wordsplit.h>
#include <mailutils/debug.h>
#include <mailutils/errno.h>
#include <mailutils/header.h>
#include <mailutils/body.h>
#include <mailutils/iterator.h>
#include <mailutils/message.h>
#include <mailutils/mime.h>
#include <mailutils/util.h>
#include <mailutils/observer.h>
#include <mailutils/property.h>
#include <mailutils/stream.h>
#include <mailutils/smtp.h>
#include <mailutils/tls.h>
#include <mailutils/cstr.h>
#include <mailutils/sockaddr.h>
#include <mailutils/sys/mailer.h>
#include <mailutils/sys/url.h>
#include <mailutils/sys/registrar.h>

static int      _mailer_smtp_init (mu_mailer_t);

static int
_url_smtp_init (mu_url_t url)
{
  if (url->port == 0)
    url->port = MU_SMTP_PORT;
  return 0;
}

static struct _mu_record _smtp_record = {
  MU_SMTP_PRIO,
  MU_SMTP_SCHEME,
  MU_RECORD_DEFAULT,
  MU_URL_SCHEME | MU_URL_CRED | MU_URL_INET | MU_URL_PARAM,
  MU_URL_HOST,
  _url_smtp_init,		/* url init.  */
  _mu_mailer_mailbox_init,	/* Mailbox init.  */
  _mailer_smtp_init,		/* Mailer init.  */
  _mu_mailer_folder_init,	/* Folder init.  */
  NULL,				/* No need for a back pointer.  */
  NULL,				/* _is_scheme method.  */
  NULL,				/* _get_url method.  */
  NULL,				/* _get_mailbox method.  */
  NULL,				/* _get_mailer method.  */
  NULL				/* _get_folder method.  */
};

/* We export : url parsing and the initialisation of
   the mailbox, via the register entry/record.  */
mu_record_t     mu_smtp_record = &_smtp_record;

#ifdef WITH_TLS
static struct _mu_record _smtps_record = {
  MU_SMTP_PRIO,
  MU_SMTPS_SCHEME,
  MU_RECORD_DEFAULT,
  MU_URL_SCHEME | MU_URL_CRED | MU_URL_INET | MU_URL_PARAM,
  MU_URL_HOST,
  _url_smtp_init,		/* url init.  */
  _mu_mailer_mailbox_init,	/* Mailbox init.  */
  _mailer_smtp_init,		/* Mailer init.  */
  _mu_mailer_folder_init,	/* Folder init.  */
  NULL,				/* No need for a back pointer.  */
  NULL,				/* _is_scheme method.  */
  NULL,				/* _get_url method.  */
  NULL,				/* _get_mailbox method.  */
  NULL,				/* _get_mailer method.  */
  NULL				/* _get_folder method.  */
};
mu_record_t     mu_smtps_record = &_smtps_record;
#endif

struct _smtp_mailer
{
  mu_mailer_t     mailer;
  mu_smtp_t       smtp;

  mu_address_t    rcpt_to;
  mu_address_t    rcpt_bcc;
};

static void
smtp_mailer_add_auth_mech (struct _smtp_mailer *smtp_mailer, const char *str)
{
  struct mu_wordsplit ws;

  ws.ws_delim = ",";
  if (mu_wordsplit (str, &ws, MU_WRDSF_DEFFLAGS|MU_WRDSF_DELIM|MU_WRDSF_WS))
    mu_error (_("cannot split line `%s': %s"), str,
	      mu_wordsplit_strerror (&ws));
  else
    {
      size_t i;
      for (i = 0; i < ws.ws_wordc; i++)
	mu_smtp_add_auth_mech (smtp_mailer->smtp, ws.ws_wordv[i]);
      mu_wordsplit_free (&ws);
    }
}

static int
smtp_open (mu_mailer_t mailer, int flags)
{
  const char *auth, *scheme;
  struct _smtp_mailer *smtp_mailer = mailer->data;
  int rc;
  size_t parmc = 0;
  char **parmv = NULL;
  int tls = 0;
  int nostarttls = 0;
  int noauth = 0;

  rc = mu_url_sget_scheme (mailer->url, &scheme);
  if (rc == 0) 
    tls = strcmp (scheme, "smtps") == 0;

  rc = mu_smtp_create (&smtp_mailer->smtp);
  if (rc)
    return rc;
  if (mu_debug_level_p (MU_DEBCAT_MAILER, MU_DEBUG_PROT))
    mu_smtp_trace (smtp_mailer->smtp, MU_SMTP_TRACE_SET);
  if (mu_debug_level_p (MU_DEBCAT_MAILER, MU_DEBUG_TRACE6))
    mu_smtp_trace_mask (smtp_mailer->smtp, MU_SMTP_TRACE_SET,
			MU_XSCRIPT_SECURE);
  if (mu_debug_level_p (MU_DEBCAT_MAILER, MU_DEBUG_TRACE7))
    mu_smtp_trace_mask (smtp_mailer->smtp, MU_SMTP_TRACE_SET,
			MU_XSCRIPT_PAYLOAD);
  
  mu_smtp_set_url (smtp_mailer->smtp, mailer->url);

  if (mu_url_sget_auth (mailer->url, &auth) == 0)
    smtp_mailer_add_auth_mech (smtp_mailer, auth);
  
  /* Additional information is supplied in the arguments */
  if (mu_url_sget_fvpairs (mailer->url, &parmc, &parmv) == 0)
    {
      size_t i;
      
      for (i = 0; i < parmc; i++)
	{
	  if (strcmp (parmv[i], "notls") == 0)
	    nostarttls = 1;
	  else if (strcmp (parmv[i], "noauth") == 0)
	    noauth = 1;
	  else if (strncmp (parmv[i], "auth=", 5) == 0)
	    smtp_mailer_add_auth_mech (smtp_mailer, parmv[i] + 5);
	  /* unrecognized arguments silently ignored */
	}
    }
  
  if (mailer->stream == NULL)
    {
      struct mu_sockaddr *sa;
      struct mu_sockaddr_hints hints;
      mu_stream_t transport;
      
      memset (&hints, 0, sizeof (hints));
      hints.flags = MU_AH_DETECT_FAMILY;
      hints.port = tls ? MU_SMTPS_PORT : MU_SMTP_PORT;
      hints.protocol = IPPROTO_TCP;
      hints.socktype = SOCK_STREAM;
      rc = mu_sockaddr_from_url (&sa, mailer->url, &hints);
      if (rc)
	return rc;
      
      rc = mu_tcp_stream_create_from_sa (&transport, sa, NULL, mailer->flags);
      if (rc)
        {
          mu_sockaddr_free (sa);
	  return rc;
	}
#ifdef WITH_TLS
      if (tls)
	{
	  mu_stream_t tlsstream;
	  
	  rc = mu_tls_client_stream_create (&tlsstream, transport, transport,
					    0);
	  mu_stream_unref (transport);
	  if (rc)
	    {
	      mu_debug (MU_DEBCAT_MAILER, MU_DEBUG_ERROR,
			(_("cannot create TLS stream: %s"),
			 mu_strerror (rc)));
	      mu_sockaddr_free (sa);
	      if (mu_tls_enable)
		return rc;
	    }
	  else
	    transport = tlsstream;
	  nostarttls = 1;
	}
#endif
      mailer->stream = transport;
      mu_stream_set_buffer (mailer->stream, mu_buffer_line, 0);
    }
  mu_smtp_set_carrier (smtp_mailer->smtp, mailer->stream);
  /* FIXME: Unref the stream */

  rc = mu_smtp_open (smtp_mailer->smtp);
  if (rc)
    return rc;

  rc = mu_smtp_ehlo (smtp_mailer->smtp);
  if (rc)
    return rc;

#ifdef WITH_TLS
  if (!nostarttls &&
      mu_smtp_capa_test (smtp_mailer->smtp, "STARTTLS", NULL) == 0)
    {
      rc = mu_smtp_starttls (smtp_mailer->smtp);
      if (rc == 0)
	{
	  rc = mu_smtp_ehlo (smtp_mailer->smtp);
	  if (rc)
	    return rc;
	}
    }
#endif
  if (!noauth && mu_smtp_capa_test (smtp_mailer->smtp, "AUTH", NULL) == 0)
    {
      rc = mu_smtp_auth (smtp_mailer->smtp);
      switch (rc)
	{
	case 0:
	  rc = mu_smtp_ehlo (smtp_mailer->smtp);
	  break;
	  
	case ENOSYS:
	case MU_ERR_AUTH_NO_CRED:
	  mu_diag_output (MU_DIAG_NOTICE, "authentication disabled: %s",
			  mu_strerror (rc));
	  rc = 0; /* Continue anyway */
	  break;
	}
      if (rc)
	return rc;
    }
  
  return 0;
}

static void
smtp_destroy (mu_mailer_t mailer)
{
  struct _smtp_mailer *smp = mailer->data;
  
  mu_address_destroy (&smp->rcpt_to);
  mu_address_destroy (&smp->rcpt_bcc);
  mu_smtp_destroy (&smp->smtp);

  free (smp);

  mailer->data = NULL;
}

static int
smtp_close (mu_mailer_t mailer)
{
  struct _smtp_mailer *smp = mailer->data;
  return mu_smtp_quit (smp->smtp);
}


int
smtp_address_add (mu_address_t *paddr, const char *value)
{
  mu_address_t addr = NULL;
  int status;

  status = mu_address_create (&addr, value);
  if (status)
    return status;
  status = mu_address_union (paddr, addr);
  mu_address_destroy (&addr);
  return status;
}

static int
_smtp_property_is_set (struct _smtp_mailer *smp, const char *name)
{
  mu_property_t property = NULL;

  mu_mailer_get_property (smp->mailer, &property);
  return mu_property_is_set (property, name);
}


static int
_smtp_set_rcpt (struct _smtp_mailer *smp, mu_message_t msg, mu_address_t to)
{
  int             status = 0;

  /* Get RCPT_TO from TO, or the message. */

  if (to)
    {
      /* Use the specified mu_address_t. */
      if ((status = mu_mailer_check_to (to)) != 0)
	{
	  mu_debug (MU_DEBCAT_MAILER, MU_DEBUG_ERROR,
		    ("mu_mailer_send_message(): explicit to not valid"));
	  return status;
	}
      smp->rcpt_to = mu_address_dup (to);
      if (!smp->rcpt_to)
        return ENOMEM;

      if (status)
	return status;
    }

  if (!to || _smtp_property_is_set (smp, "READ_RECIPIENTS"))
    {
      mu_header_t     header;
      
      if ((status = mu_message_get_header (msg, &header)))
	return status;

      do
	{
	  const char      *value;
	  
	  status = mu_header_sget_value (header, MU_HEADER_TO, &value);
	  if (status == 0)
	    smtp_address_add (&smp->rcpt_to, value);
	  else if (status != MU_ERR_NOENT)
	    break;

	  status = mu_header_sget_value (header, MU_HEADER_CC, &value);
	  if (status == 0)
	    smtp_address_add (&smp->rcpt_to, value);
	  else if (status != MU_ERR_NOENT)
	    break;

	  status = mu_header_sget_value (header, MU_HEADER_BCC, &value);
	  if (status == 0)
	    smtp_address_add (&smp->rcpt_bcc, value);
	  else if (status != MU_ERR_NOENT)
	    break;

	  if (smp->rcpt_to && (status = mu_mailer_check_to (smp->rcpt_to)))
	    break;

	  if (smp->rcpt_bcc && (status = mu_mailer_check_to (smp->rcpt_bcc)))
	    break;
	}
      while (0);
    }

  if (status)
    {
      mu_address_destroy (&smp->rcpt_to);
      mu_address_destroy (&smp->rcpt_bcc);
    }
  else
    {
      size_t rcpt_cnt, bcc_cnt;
      
      if (smp->rcpt_to)
	mu_address_get_count (smp->rcpt_to, &rcpt_cnt);
      
      if (smp->rcpt_bcc)
	mu_address_get_count (smp->rcpt_bcc, &bcc_cnt);

      if (rcpt_cnt + bcc_cnt == 0)
	status = MU_ERR_MAILER_NO_RCPT_TO;
    }

  return status;
}

static int
_rcpt_to_addr (mu_smtp_t smtp, mu_address_t addr, size_t *pcount)
{
  size_t i, count, rcpt_cnt = 0;
  int status;
  
  status = mu_address_get_count (addr, &count);
  if (status)
    return status;

  for (i = 1; i <= count; i++)
    {
      const char *to = NULL;

      status = mu_address_sget_email (addr, i, &to);
      if (status == 0)
	{
	  status = mu_smtp_rcpt_basic (smtp, to, NULL);
	  if (status == 0)
	    rcpt_cnt++;
	  else if (status != MU_ERR_REPLY)
	    break;
	}
    }
  *pcount = rcpt_cnt;
  return status;
}

static int
smtp_send_message (mu_mailer_t mailer, mu_message_t msg,
		   mu_address_t argfrom, mu_address_t argto)
{
  struct _smtp_mailer *smp;
  mu_smtp_t smtp;
  int status;
  size_t size, lines, count;
  const char *mail_from, *size_str;
  mu_header_t     header;
      
  if (mailer == NULL)
    return EINVAL;
  
  smp = mailer->data;
  if (!smp)
    return EINVAL;
  smtp = smp->smtp;
  
  if ((status = mu_message_get_header (msg, &header)))
    return status;
  
  status = _smtp_set_rcpt (smp, msg, argto);
  if (status)
    return status;

  status = mu_address_sget_email (argfrom, 1, &mail_from);
  if (status)
    return status;
  
  if (mu_smtp_capa_test (smtp, "SIZE", &size_str) == 0 &&
      mu_message_size (msg, &size) == 0 &&
      mu_message_lines (msg, &lines) == 0)
    {
      size_t msgsize = size + lines;
      if (strncmp (size_str, "SIZE ", 5) == 0)
	{
	  size_t maxsize = strtoul (size_str + 5, NULL, 10);

	  if (msgsize && maxsize && msgsize > maxsize)
	    return EFBIG;
	}
      status = mu_smtp_mail_basic (smtp, mail_from,
				   "SIZE=%lu",
				   (unsigned long) msgsize);
    }
  else
    status = mu_smtp_mail_basic (smtp, mail_from, NULL);
  if (status)
    {
      if (status == MU_ERR_REPLY)
	mu_smtp_rset (smtp);
      return status;
    }

  status = _rcpt_to_addr (smtp, smp->rcpt_to, &count);
  if (status && count == 0)
    {
      if (status == MU_ERR_REPLY)
	mu_smtp_rset (smtp);
      return status;
    }
  status = _rcpt_to_addr (smtp, smp->rcpt_bcc, &count);
  if (status && count == 0)
    {
      if (status == MU_ERR_REPLY)
	mu_smtp_rset (smtp);
      return status;
    }

  if (mu_header_sget_value (header, MU_HEADER_BCC, NULL) == 0||
      mu_header_sget_value (header, MU_HEADER_FCC, NULL) == 0)
    {
      mu_iterator_t itr;
      mu_body_t body;
      mu_stream_t ostr, bstr;

      status = mu_smtp_data (smtp, &ostr);
      if (status)
	{
	  if (status == MU_ERR_REPLY)
	    mu_smtp_rset (smtp);
	  return status;
	}
      mu_header_get_iterator (header, &itr);
      for (mu_iterator_first (itr); !mu_iterator_is_done (itr);
	   mu_iterator_next (itr))
	{
	  const char *name;
	  void *value;

	  mu_iterator_current_kv (itr, (void*) &name, &value);
	  if (mu_c_strcasecmp (name, MU_HEADER_BCC) == 0 ||
	      mu_c_strcasecmp (name, MU_HEADER_FCC) == 0)
	    continue;
	  mu_stream_printf (ostr, "%s: %s\n", name, (char*)value);
	}
      mu_iterator_destroy (&itr);
      mu_stream_write (ostr, "\n", 1, NULL);
      
      mu_message_get_body (msg, &body);
      mu_body_get_streamref (body, &bstr);
      status = mu_stream_copy (ostr, bstr, 0, NULL);
      mu_stream_destroy (&bstr);
      mu_stream_close (ostr);
      mu_stream_destroy (&ostr);
    }
  else
    {
      mu_stream_t str;
      mu_message_get_streamref (msg, &str);
      status = mu_smtp_send_stream (smtp, str);
      mu_stream_destroy (&str);
    }
  mu_address_destroy (&smp->rcpt_to);
  mu_address_destroy (&smp->rcpt_bcc);
  if (status == 0)
    {
      status = mu_smtp_dot (smtp);
      if (status == MU_ERR_REPLY)
	mu_smtp_rset (smtp);
    }
  return status;
}

static int
_mailer_smtp_init (mu_mailer_t mailer)
{
  struct _smtp_mailer *smp;

  /* Allocate memory specific to smtp mailer.  */
  smp = mailer->data = calloc (1, sizeof (*smp));
  if (mailer->data == NULL)
    return ENOMEM;

  smp->mailer = mailer;	/* Back pointer.  */

  mailer->_destroy = smtp_destroy;
  mailer->_open = smtp_open;
  mailer->_close = smtp_close;
  mailer->_send_message = smtp_send_message;

  /* Set our properties.  */
  {
    mu_property_t   property = NULL;

    mu_mailer_get_property (mailer, &property);
    mu_property_set_value (property, "TYPE", "SMTP", 1);
  }

  return 0;
}

#else
#include <stdio.h>
#include <mailutils/sys/registrar.h>
mu_record_t     mu_smtp_record = NULL;
mu_record_t     mu_remote_smtp_record = NULL;
#endif
