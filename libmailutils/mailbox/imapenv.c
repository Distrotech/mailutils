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
#include <mailutils/types.h>
#include <mailutils/header.h>
#include <mailutils/envelope.h>
#include <mailutils/address.h>
#include <mailutils/message.h>
#include <mailutils/sys/message.h>
#include <mailutils/errno.h>

void
mu_message_imapenvelope_free (struct mu_imapenvelope *env)
{
  if (!env)
    return;
  free (env->subject);
  mu_address_destroy (&env->from);
  mu_address_destroy (&env->sender);
  mu_address_destroy (&env->reply_to);
  mu_address_destroy (&env->to);
  mu_address_destroy (&env->cc);
  mu_address_destroy (&env->bcc);
  free (env->in_reply_to);
  free (env->message_id);
  free (env);
}

int
mu_message_get_imapenvelope (mu_message_t msg, struct mu_imapenvelope **pimapenvelope)
{
  struct mu_imapenvelope *imapenvelope;
  int rc;
  
  if (msg == NULL)
    return EINVAL;
  if (pimapenvelope == NULL)
    return MU_ERR_OUT_PTR_NULL;
  if (msg->_imapenvelope)
    return msg->_imapenvelope (msg, pimapenvelope);

  imapenvelope = calloc (1, sizeof (imapenvelope[0]));
  if (!imapenvelope)
    return ENOMEM;
  do
    {
      mu_header_t hdr;
      mu_envelope_t env;
      const char *s;

      if ((rc = mu_message_get_envelope (msg, &env)))
	break;
      if ((rc = mu_envelope_sget_date (env, &s)))
	break;
      if ((rc = mu_scan_datetime (s, MU_DATETIME_FROM,
				  &imapenvelope->date, &imapenvelope->tz,
				  NULL)))
	break;

      if ((rc = mu_message_get_header (msg, &hdr)))
	break;

      rc = mu_header_get_address (hdr, MU_HEADER_FROM, &imapenvelope->from);
      if (rc && rc != MU_ERR_NOENT)
	break;
      
      rc = mu_header_get_address (hdr, MU_HEADER_SENDER, &imapenvelope->sender);
      if (rc && rc != MU_ERR_NOENT)
	break;

      rc = mu_header_get_address (hdr, MU_HEADER_REPLY_TO, &imapenvelope->reply_to);
      if (rc && rc != MU_ERR_NOENT)
	break;

      rc = mu_header_get_address (hdr, MU_HEADER_TO, &imapenvelope->to);
      if (rc && rc != MU_ERR_NOENT)
	break;

      rc = mu_header_get_address (hdr, MU_HEADER_CC, &imapenvelope->cc);
      if (rc && rc != MU_ERR_NOENT)
	break;

      rc = mu_header_get_address (hdr, MU_HEADER_BCC, &imapenvelope->bcc);
      if (rc && rc != MU_ERR_NOENT)
	break;
      
      rc = mu_header_aget_value_unfold (hdr, MU_HEADER_SUBJECT,
					&imapenvelope->subject);
      if (rc && rc != MU_ERR_NOENT)
	break;

      rc = mu_header_aget_value_unfold (hdr, MU_HEADER_IN_REPLY_TO,
					&imapenvelope->in_reply_to);
      if (rc && rc != MU_ERR_NOENT)
	break;

      rc = mu_header_aget_value_unfold (hdr, MU_HEADER_MESSAGE_ID,
					&imapenvelope->message_id);
    }
  while (0);
  if (rc)
    mu_message_imapenvelope_free (imapenvelope);
  else
    *pimapenvelope = imapenvelope;
  return rc;
}

int
mu_message_set_imapenvelope (mu_message_t msg,
			int (*_imapenvelope) (mu_message_t,
					 struct mu_imapenvelope **),
			void *owner)
{
  if (msg == NULL)
    return EINVAL;
  if (msg->owner != owner)
    return EACCES;
  msg->_imapenvelope = _imapenvelope;
  return 0;
}
