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
#include <mailutils/assoc.h>
#include <mailutils/stream.h>
#include <mailutils/imap.h>
#include <mailutils/imaputil.h>
#include <mailutils/sys/imap.h>

int
mu_imap_store_flags (mu_imap_t imap, int uid, mu_msgset_t msgset,
		     int op, int flags)
{
  int status;
  static char *cmd[] = { "FLAGS", "+FLAGS", "-FLAGS" };
  
  if (imap == NULL || (op & MU_IMAP_STORE_OPMASK) >= MU_ARRAY_SIZE (cmd))
    return EINVAL;
  if (!imap->io)
    return MU_ERR_NO_TRANSPORT;
  if (imap->session_state < MU_IMAP_SESSION_SELECTED)
    return MU_ERR_SEQ;

  switch (imap->client_state)
    {
    case MU_IMAP_CLIENT_READY:
      status = _mu_imap_tag_next (imap);
      MU_IMAP_CHECK_EAGAIN (imap, status);
      mu_imapio_printf (imap->io, "%s ", imap->tag_str);
      if (uid)
	mu_imapio_printf (imap->io, "UID ");
      mu_imapio_printf (imap->io, "STORE ");
      mu_imapio_send_msgset (imap->io, msgset);
      mu_imapio_printf (imap->io, " %s", cmd[op & MU_IMAP_STORE_OPMASK]);
      if (op & MU_IMAP_STORE_SILENT)
	mu_imapio_printf (imap->io, ".SILENT");
      mu_imapio_printf (imap->io, " ");
      mu_imapio_send_flags (imap->io, flags);
      mu_imapio_printf (imap->io, "\r\n");
      status = mu_imapio_last_error (imap->io);
      MU_IMAP_CHECK_ERROR (imap, status);
      MU_IMAP_FCLR (imap, MU_IMAP_RESP);
      imap->client_state = MU_IMAP_CLIENT_STORE_RX;
      
    case MU_IMAP_CLIENT_STORE_RX:
      /* FIXME: Handle unsolicited responses */
      status = _mu_imap_response (imap, NULL, NULL);
      MU_IMAP_CHECK_EAGAIN (imap, status);
      switch (imap->response)
	{
	case MU_IMAP_OK:
	  status = 0;
	  break;
	  
	case MU_IMAP_NO:
	  status = MU_ERR_FAILURE;
	  break;

	case MU_IMAP_BAD:
	  status = MU_ERR_BADREPLY;
	  break;
	}
      imap->client_state = MU_IMAP_CLIENT_READY;
      break;

    default:
      status = EINPROGRESS;
    }
  return status;
}
