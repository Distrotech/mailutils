/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2002, 2004-2007, 2009-2012 Free Software
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

#include <config.h>
#include <sys/types.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

#include <mailutils/types.h>
#include <mailutils/message.h>
#include <mailutils/errno.h>
#include <mailutils/envelope.h>
#include <mailutils/util.h>
#include <mailutils/datetime.h>
#include <mailutils/header.h>
#include <mailutils/mu_auth.h>
#include <mailutils/address.h>
#include <mailutils/sys/message.h>

/* Message envelope */
static int
message_envelope_date (mu_envelope_t envelope, char *buf, size_t len,
		       size_t *pnwrite)
{
  mu_message_t msg = mu_envelope_get_owner (envelope);
  time_t t;
  size_t n;

  if (msg == NULL)
    return EINVAL;

  /* FIXME: extract the time from "Date:".  */

  if (buf == NULL || len == 0)
    {
      n = MU_DATETIME_FROM_LENGTH;
    }
  else
    {
      char tmpbuf[MU_DATETIME_FROM_LENGTH+1];
      t = time (NULL);
      n = mu_strftime (tmpbuf, sizeof tmpbuf, 
                       MU_DATETIME_FROM, gmtime (&t));
      n = mu_cpystr (buf, tmpbuf, len);
    }
  if (pnwrite)
    *pnwrite = n;
  return 0;
}

static int
message_envelope_sender (mu_envelope_t envelope, char *buf, size_t len,
			 size_t *pnwrite)
{
  mu_message_t msg = mu_envelope_get_owner (envelope);
  mu_header_t header;
  int status;
  const char *sender;
  struct mu_auth_data *auth = NULL;
  static char *hdrnames[] = {
    "X-Envelope-Sender",
    "X-Envelope-From",
    "X-Original-Sender",
    "From",
    NULL
  };
  mu_address_t address = NULL;

  if (msg == NULL)
    return EINVAL;

  /* First, try the header  */
  status = mu_message_get_header (msg, &header);
  if (status)
    return status;
  status = mu_header_sget_firstof (header, hdrnames, &sender, NULL);
  if (status)
    {
      auth = mu_get_auth_by_uid (getuid ());
      if (!auth)
	return MU_ERR_NOENT;
      sender = auth->name;
    }

  status = mu_address_create (&address, sender);
  if (status == 0)
    {
      status = mu_address_sget_email (address, 1, &sender);
      if (status == 0)
	{
	  size_t n = strlen (sender);
	  if (buf && len > 0)
	    {
	      len--; /* One for the null.  */
	      n = (n < len) ? n : len;
	      memcpy (buf, sender, n);
	      buf[n] = '\0';
	    }
	  if (pnwrite)
	    *pnwrite = n;
	}
      mu_address_destroy (&address);
    }
  
  if (auth)
    mu_auth_data_free (auth);

  return status;
}

int
mu_message_get_envelope (mu_message_t msg, mu_envelope_t *penvelope)
{
  if (msg == NULL)
    return EINVAL;
  if (penvelope == NULL)
    return MU_ERR_OUT_PTR_NULL;

  if (msg->envelope == NULL)
    {
      mu_envelope_t envelope;
      int status = mu_envelope_create (&envelope, msg);
      if (status != 0)
	return status;
      mu_envelope_set_sender (envelope, message_envelope_sender, msg);
      mu_envelope_set_date (envelope, message_envelope_date, msg);
      msg->envelope = envelope;
    }
  *penvelope = msg->envelope;
  return 0;
}

int
mu_message_set_envelope (mu_message_t msg, mu_envelope_t envelope, void *owner)
{
  if (msg == NULL)
    return EINVAL;
  if (msg->owner != owner)
    return EACCES;
  if (msg->envelope)
    mu_envelope_destroy (&msg->envelope, msg);
  msg->envelope = envelope;
  msg->flags |= MESSAGE_MODIFIED;
  return 0;
}
