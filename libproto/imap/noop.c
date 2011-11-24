/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2011 Free Software Foundation, Inc.

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
#include <mailutils/errno.h>
#include <mailutils/imap.h>
#include <mailutils/sys/imap.h>

int
mu_imap_noop (mu_imap_t imap)
{
  int status;
  
  if (imap == NULL)
    return EINVAL;
  if (!imap->io)
    return MU_ERR_NO_TRANSPORT;
  if (imap->state != MU_IMAP_CONNECTED)
    return MU_ERR_SEQ;

  switch (imap->state)
    {
    case MU_IMAP_CONNECTED:
      status = _mu_imap_tag_next (imap);
      MU_IMAP_CHECK_EAGAIN (imap, status);
      status = mu_imapio_printf (imap->io, "%s NOOP\r\n", imap->tag_str);
      MU_IMAP_CHECK_ERROR (imap, status);
      MU_IMAP_FCLR (imap, MU_IMAP_RESP);
      imap->state = MU_IMAP_NOOP_RX;

    case MU_IMAP_NOOP_RX:
      status = _mu_imap_response (imap, NULL, NULL);
      MU_IMAP_CHECK_EAGAIN (imap, status);
      switch (imap->resp_code)
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
      imap->state = MU_IMAP_CONNECTED;
      break;

    default:
      status = EINPROGRESS;
    }
  return status;
}
      
