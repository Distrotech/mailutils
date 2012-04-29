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
#include <mailutils/tls.h>
#include <mailutils/imap.h>
#include <mailutils/sys/imap.h>

int
mu_imap_starttls (mu_imap_t imap)
{
#ifdef WITH_TLS
  int status;
  mu_stream_t tlsstream, streams[2];
  
  if (imap == NULL)
    return EINVAL;
  if (!imap->io)
    return MU_ERR_NO_TRANSPORT;
  if (imap->session_state == MU_IMAP_SESSION_INIT)
    return MU_ERR_SEQ;

  status = mu_imap_capability_test (imap, "STARTTLS", NULL);
  if (status == MU_ERR_NOENT)
    return ENOSYS;
  else if (status)
    return status;
  
  switch (imap->client_state)
    {
    case MU_IMAP_CLIENT_READY:
      status = _mu_imap_tag_next (imap);
      MU_IMAP_CHECK_EAGAIN (imap, status);
      status = mu_imapio_printf (imap->io, "%s STARTTLS\r\n", imap->tag_str);
      MU_IMAP_CHECK_ERROR (imap, status);
      MU_IMAP_FCLR (imap, MU_IMAP_RESP);
      imap->client_state = MU_IMAP_CLIENT_STARTTLS_RX;
      
    case MU_IMAP_CLIENT_STARTTLS_RX:
      status = _mu_imap_response (imap, NULL, NULL);
      MU_IMAP_CHECK_EAGAIN (imap, status);
      switch (imap->response)
	{
	case MU_IMAP_OK:
	  status = mu_imapio_get_streams (imap->io, streams);
	  MU_IMAP_CHECK_EAGAIN (imap, status);
	  status = mu_tls_client_stream_create (&tlsstream,
						streams[0], streams[1], 0);
	  mu_stream_unref (streams[0]);
	  mu_stream_unref (streams[1]);
	  MU_IMAP_CHECK_EAGAIN (imap, status);
	  streams[0] = streams[1] = tlsstream;
	  status = mu_imapio_set_streams (imap->io, streams);
	  mu_stream_unref (streams[0]);
	  mu_stream_unref (streams[1]);
	  MU_IMAP_CHECK_EAGAIN (imap, status);
	  /* Invalidate the capability list */
	  mu_list_destroy (&imap->capa);
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
#else
  return ENOSYS;
#endif
}

