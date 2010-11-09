/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2010 Free Software Foundation, Inc.

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

#include <mailutils/errno.h>
#include <mailutils/stream.h>
#include <mailutils/sys/imap.h>

int
mu_imap_login (mu_imap_t imap, const char *user, const char *pass)
{
  int status;
  
  if (imap == NULL)
    return EINVAL;
  if (!imap->carrier)
    return MU_ERR_NO_TRANSPORT;
  if (imap->state != MU_IMAP_CONNECTED)
    return MU_ERR_SEQ;
  if (imap->imap_state != MU_IMAP_STATE_NONAUTH)
    return MU_ERR_SEQ;
  
  switch (imap->state)
    {
    case MU_IMAP_CONNECTED:
      if (mu_imap_trace_mask (imap, MU_IMAP_TRACE_QRY, MU_XSCRIPT_SECURE))
	_mu_imap_xscript_level (imap, MU_XSCRIPT_SECURE);
      status = _mu_imap_tag_next (imap);
      MU_IMAP_CHECK_EAGAIN (imap, status);
      status = mu_stream_printf (imap->carrier, "%s LOGIN \"%s\" \"%s\"\r\n",
				 imap->tag_str, user, pass);
      _mu_imap_xscript_level (imap, MU_XSCRIPT_NORMAL);
      /* FIXME: how to obscure the passwd in the stream buffer? */
      MU_IMAP_CHECK_EAGAIN (imap, status);
      MU_IMAP_FCLR (imap, MU_IMAP_RESP);
      imap->state = MU_IMAP_LOGIN_RX;

    case MU_IMAP_LOGIN_RX:
      status = _mu_imap_response (imap);
      MU_IMAP_CHECK_EAGAIN (imap, status);
      switch (imap->resp_code)
	{
	case MU_IMAP_OK:
	  imap->imap_state = MU_IMAP_STATE_AUTH;
	  break;

	case MU_IMAP_NO:
	  status = EACCES;
	  break;

	case MU_IMAP_BAD:
	  status = MU_ERR_BADREPLY;
	  break;
	}
      imap->state = MU_IMAP_CONNECTED;
      break;

    default:
      status = EINPROGRESS;
    }
  return status;
}

