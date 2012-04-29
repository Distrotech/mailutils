/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2010-2012 Free Software Foundation, Inc.

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
#include <mailutils/secret.h>
#include <mailutils/sys/imap.h>

int
mu_imap_login (mu_imap_t imap, const char *user, const char *pass)
{
  int status;
  
  if (imap == NULL)
    return EINVAL;
  if (!imap->io)
    return MU_ERR_NO_TRANSPORT;
  if (imap->session_state != MU_IMAP_SESSION_NONAUTH)
    return MU_ERR_SEQ;
  
  switch (imap->client_state)
    {
    case MU_IMAP_CLIENT_READY:
      if (mu_imap_trace_mask (imap, MU_IMAP_TRACE_QRY, MU_XSCRIPT_SECURE))
	_mu_imap_xscript_level (imap, MU_XSCRIPT_SECURE);
      status = _mu_imap_tag_next (imap);
      MU_IMAP_CHECK_EAGAIN (imap, status);
      status = mu_imapio_send_command (imap->io, imap->tag_str, NULL,
				       "LOGIN", user, pass, NULL);
      _mu_imap_xscript_level (imap, MU_XSCRIPT_NORMAL);
      /* FIXME: how to obscure the passwd in the stream buffer? */
      MU_IMAP_CHECK_EAGAIN (imap, status);
      MU_IMAP_FCLR (imap, MU_IMAP_RESP);
      imap->client_state = MU_IMAP_CLIENT_LOGIN_RX;

    case MU_IMAP_CLIENT_LOGIN_RX:
      status = _mu_imap_response (imap, NULL, NULL);
      imap->client_state = MU_IMAP_CLIENT_READY;
      MU_IMAP_CHECK_EAGAIN (imap, status);
      switch (imap->response)
	{
	case MU_IMAP_OK:
	  imap->session_state = MU_IMAP_SESSION_AUTH;
	  break;

	case MU_IMAP_NO:
	  status = EACCES;
	  break;

	case MU_IMAP_BAD:
	  status = MU_ERR_BADREPLY;
	  break;
	}
      break;

    default:
      status = EINPROGRESS;
    }
  return status;
}

int
mu_imap_login_secret (mu_imap_t imap, const char *user, mu_secret_t secret)
{
  int rc = mu_imap_login (imap, user, mu_secret_password (secret));
  mu_secret_password_unref (secret);
  return rc;
}
  
  
