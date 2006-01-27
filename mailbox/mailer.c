/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2004, 2005, 2006
   Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library; if not, write to the
   Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301 USA */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

#include <mailutils/address.h>
#include <mailutils/debug.h>
#include <mailutils/errno.h>
#include <mailutils/iterator.h>
#include <mailutils/list.h>
#include <mailutils/observer.h>
#include <mailutils/property.h>
#include <mailutils/registrar.h>
#include <mailutils/stream.h>
#include <mailutils/url.h>
#include <mailutils/header.h>
#include <mailutils/mailbox.h>
#include <mailutils/message.h>

#include <mailer0.h>

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
mu_mailer_create (mu_mailer_t * pmailer, const char *name)
{
  mu_record_t record;

  if (pmailer == NULL)
    return MU_ERR_OUT_PTR_NULL;

  if (name == NULL)
    mu_mailer_get_url_default (&name);

  if (mu_registrar_lookup (name, MU_FOLDER_ATTRIBUTE_FILE, &record, NULL) == 0)
    {
      int (*m_init) (mu_mailer_t) = NULL;
      int (*u_init) (mu_url_t) = NULL;

      mu_record_get_mailer (record, &m_init);
      mu_record_get_url (record, &u_init);
      if (m_init && u_init)
        {
	  int status;
	  mu_url_t url;
	  mu_mailer_t mailer;
	  
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

	  /* Parse the url, it may be a bad one and we should bail out if this
	     failed.  */
	  if ((status = mu_url_create (&url, name)) != 0
	      || (status = u_init (url)) != 0)
	    {
	      mu_mailer_destroy (&mailer);
	      return status;
	    }
	  mailer->url = url;

	  status = m_init (mailer);
	  if (status)
	    mu_mailer_destroy (&mailer);
	  else
	    *pmailer = mailer;

	  return status;
	}
    }

    return MU_ERR_MAILER_BAD_URL;
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
	  mu_observable_notify (mailer->observable, MU_EVT_MAILER_DESTROY);
	  mu_observable_destroy (&(mailer->observable), mailer);
	}

      /* Call the object destructor.  */
      if (mailer->_destroy)
	mailer->_destroy (mailer);

      mu_monitor_wrlock (monitor);

      if (mailer->stream)
	{
	  /* FIXME: Should be the client responsability to close this?  */
	  /* mu_stream_close (mailer->stream); */
	  mu_stream_destroy (&(mailer->stream), mailer);
	}

      if (mailer->url)
	mu_url_destroy (&(mailer->url));

      if (mailer->debug)
	mu_debug_destroy (&(mailer->debug), mailer);

      if (mailer->property)
	mu_property_destroy (&(mailer->property), mailer);

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
  char buf[512];
  
  if (mu_message_get_header (msg, &hdr))
    return;

  if (mu_header_get_value (hdr, MU_HEADER_FCC, NULL, 0, NULL))
    return;
  
  mu_header_get_field_count (hdr, &count);
  for (i = 1; i <= count; i++)
    {
      mu_mailbox_t mbox;
      
      mu_header_get_field_name (hdr, i, buf, sizeof buf, NULL);
      if (strcasecmp (buf, MU_HEADER_FCC) == 0)
	{
	  if (mu_header_get_field_value (hdr, i, buf, sizeof buf, NULL))
	    continue;
	  if (mu_mailbox_create_default (&mbox, buf))
	    continue; /*FIXME: error message?? */
	  if (mu_mailbox_open (mbox, MU_STREAM_RDWR|MU_STREAM_CREAT|MU_STREAM_APPEND) == 0)
	    {
	      mu_mailbox_append_message (mbox, msg);
	      mu_mailbox_flush (mbox, 0);
	    }
	  mu_mailbox_close (mbox);
	  mu_mailbox_destroy (&mbox);
	}
    }
}

int
mu_mailer_send_message (mu_mailer_t mailer, mu_message_t msg,
		     mu_address_t from, mu_address_t to)
{
  int status;

  if (mailer == NULL || mailer->_send_message == NULL)
    return ENOSYS;

  /* Common API checking. */

  /* FIXME: this should be done in the concrete APIs, sendmail doesn't
     yet, though, so do it here. */
  if (from)
    {
      if ((status = mu_mailer_check_from (from)) != 0)
	return status;
    }

  if (to)
    {
      if ((status = mu_mailer_check_to (to)) != 0)
	return status;
    }
  
  save_fcc (msg);
  return mailer->_send_message (mailer, msg, from, to);
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
  if (mailer == NULL)
    return EINVAL;
  if (pstream == NULL)
    return MU_ERR_OUT_PTR_NULL;
  *pstream = mailer->stream;
  return 0;
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
mu_mailer_get_property (mu_mailer_t mailer, mu_property_t * pproperty)
{
  if (mailer == NULL)
    return EINVAL;
  if (pproperty == NULL)
    return MU_ERR_OUT_PTR_NULL;
  if (mailer->property == NULL)
    {
      int status = mu_property_create (&(mailer->property), mailer);
      if (status != 0)
	return status;
    }
  *pproperty = mailer->property;
  return 0;
}

int
mu_mailer_set_debug (mu_mailer_t mailer, mu_debug_t debug)
{
  if (mailer == NULL)
    return EINVAL;
  mu_debug_destroy (&(mailer->debug), mailer);
  mailer->debug = debug;
  return 0;
}

int
mu_mailer_get_debug (mu_mailer_t mailer, mu_debug_t * pdebug)
{
  if (mailer == NULL)
    return EINVAL;
  if (pdebug == NULL)
    return MU_ERR_OUT_PTR_NULL;
  if (mailer->debug == NULL)
    {
      int status = mu_debug_create (&(mailer->debug), mailer);
      if (status != 0)
	return status;
    }
  *pdebug = mailer->debug;
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
