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
#include <mailutils/observer.h>
#include <mailutils/property.h>
#include <mailutils/registrar.h>
#include <mailutils/stream.h>
#include <mailutils/url.h>

#include <mailer0.h>

static char *mailer_url_default;

/* FIXME: I'd like to check that the URL is valid, but that requires that the
   mailers already be registered! */
int
mailer_set_url_default (const char *url)
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
mailer_get_url_default (const char **url)
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
mailer_create (mailer_t * pmailer, const char *name)
{
  int status = EINVAL;
  url_t url = NULL;
  mailer_t mailer = NULL;

  record_t record = NULL;
  int (*m_init) __P ((mailer_t)) = NULL;
  int (*u_init) __P ((url_t)) = NULL;
  list_t list = NULL;
  iterator_t iterator;
  int found = 0;

  if (pmailer == NULL)
    return EINVAL;

  if (name == NULL)
    mailer_get_url_default (&name);

  registrar_get_list (&list);
  status = iterator_create (&iterator, list);
  if (status != 0)
    return status;
  for (iterator_first (iterator); !iterator_is_done (iterator);
       iterator_next (iterator))
    {
      iterator_current (iterator, (void **) &record);
      if (record_is_scheme (record, name))
	{
	  record_get_mailer (record, &m_init);
	  record_get_url (record, &u_init);
	  if (m_init && u_init)
	    {
	      found = 1;
	      break;
	    }
	}
    }
  iterator_destroy (&iterator);

  if (!found)
    return MU_ERR_MAILER_BAD_URL;

  /* Allocate memory for mailer.  */
  mailer = calloc (1, sizeof (*mailer));
  if (mailer == NULL)
    return ENOMEM;

  status = monitor_create (&(mailer->monitor), 0, mailer);
  if (status != 0)
    {
      mailer_destroy (&mailer);
      return status;
    }

  /* Parse the url, it may be a bad one and we should bail out if this
     failed.  */
  if ((status = url_create (&url, name)) != 0 || (status = u_init (url)) != 0)
    {
      mailer_destroy (&mailer);
      return status;
    }
  mailer->url = url;

  status = m_init (mailer);
  if (status != 0)
    {
      mailer_destroy (&mailer);
    }
  else
    *pmailer = mailer;

  return status;
}

void
mailer_destroy (mailer_t * pmailer)
{
  if (pmailer && *pmailer)
    {
      mailer_t mailer = *pmailer;
      monitor_t monitor = mailer->monitor;

      if (mailer->observable)
	{
	  observable_notify (mailer->observable, MU_EVT_MAILER_DESTROY);
	  observable_destroy (&(mailer->observable), mailer);
	}

      /* Call the object destructor.  */
      if (mailer->_destroy)
	mailer->_destroy (mailer);

      monitor_wrlock (monitor);

      if (mailer->stream)
	{
	  /* FIXME: Should be the client responsability to close this?  */
	  /* stream_close (mailer->stream); */
	  stream_destroy (&(mailer->stream), mailer);
	}

      if (mailer->url)
	url_destroy (&(mailer->url));

      if (mailer->debug)
	mu_debug_destroy (&(mailer->debug), mailer);

      if (mailer->property)
	property_destroy (&(mailer->property), mailer);

      free (mailer);
      *pmailer = NULL;
      monitor_unlock (monitor);
      monitor_destroy (&monitor, mailer);
    }
}


/* -------------- stub functions ------------------- */

int
mailer_open (mailer_t mailer, int flag)
{
  if (mailer == NULL || mailer->_open == NULL)
    return ENOSYS;
  return mailer->_open (mailer, flag);
}

int
mailer_close (mailer_t mailer)
{
  if (mailer == NULL || mailer->_close == NULL)
    return ENOSYS;
  return mailer->_close (mailer);
}


int
mailer_check_from (address_t from)
{
  size_t n = 0;

  if (!from)
    return EINVAL;

  if (address_get_count (from, &n) || n != 1)
    return MU_ERR_MAILER_BAD_FROM;

  if (address_get_email_count (from, &n) || n == 0)
    return MU_ERR_MAILER_BAD_FROM;

  return 0;
}

int
mailer_check_to (address_t to)
{
  size_t count = 0;
  size_t emails = 0;
  size_t groups = 0;

  if (!to)
    return EINVAL;

  if (address_get_count (to, &count))
    return MU_ERR_MAILER_BAD_TO;

  if (address_get_email_count (to, &emails))
    return MU_ERR_MAILER_BAD_TO;

  if (emails == 0)
    return MU_ERR_MAILER_NO_RCPT_TO;

  if (address_get_group_count (to, &groups))
    return MU_ERR_MAILER_BAD_TO;

  if (count - emails - groups != 0)
    /* then not everything is a group or an email address */
    return MU_ERR_MAILER_BAD_TO;

  return 0;
}

int
mailer_send_message (mailer_t mailer, message_t msg, address_t from, address_t to)
{
  int status;

  if (mailer == NULL || mailer->_send_message == NULL)
    return ENOSYS;

  /* Common API checking. */

  /* FIXME: this should be done in the concrete APIs, sendmail doesn't
     yet, though, so do it here. */
  if (from)
    {
      if ((status = mailer_check_from (from)) != 0)
	return status;
    }

  if (to)
    {
      if ((status = mailer_check_to (to)) != 0)
	return status;
    }

  return mailer->_send_message (mailer, msg, from, to);
}

int
mailer_set_stream (mailer_t mailer, stream_t stream)
{
  if (mailer == NULL)
    return EINVAL;
  mailer->stream = stream;
  return 0;
}

int
mailer_get_stream (mailer_t mailer, stream_t * pstream)
{
  if (mailer == NULL || pstream == NULL)
    return EINVAL;
  if (pstream)
    *pstream = mailer->stream;
  return 0;
}

int
mailer_get_observable (mailer_t mailer, observable_t * pobservable)
{
  /* FIXME: I should check for invalid types */
  if (mailer == NULL || pobservable == NULL)
    return EINVAL;

  if (mailer->observable == NULL)
    {
      int status = observable_create (&(mailer->observable), mailer);
      if (status != 0)
	return status;
    }
  *pobservable = mailer->observable;
  return 0;
}

int
mailer_get_property (mailer_t mailer, property_t * pproperty)
{
  if (mailer == NULL || pproperty == NULL)
    return EINVAL;
  if (mailer->property == NULL)
    {
      int status = property_create (&(mailer->property), mailer);
      if (status != 0)
	return status;
    }
  *pproperty = mailer->property;
  return 0;
}

int
mailer_set_debug (mailer_t mailer, mu_debug_t debug)
{
  if (mailer == NULL)
    return EINVAL;
  mu_debug_destroy (&(mailer->debug), mailer);
  mailer->debug = debug;
  return 0;
}

int
mailer_get_debug (mailer_t mailer, mu_debug_t * pdebug)
{
  if (mailer == NULL || pdebug == NULL)
    return EINVAL;
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
mailer_get_url (mailer_t mailer, url_t * purl)
{
  if (!mailer || !purl)
    return EINVAL;
  *purl = mailer->url;
  return 0;
}
