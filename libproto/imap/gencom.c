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
#include <mailutils/errno.h>
#include <mailutils/imap.h>
#include <mailutils/sys/imap.h>

int
mu_imap_gencom (mu_imap_t imap, struct imap_command *cmd)
{
  int status;
  
  if (imap == NULL || !cmd || cmd->argc < 1)
    return EINVAL;
  if (!imap->io)
    return MU_ERR_NO_TRANSPORT;
  if (imap->session_state < cmd->session_state)
    return MU_ERR_SEQ;

  if (cmd->capa)
    {
      status = mu_imap_capability_test (imap, cmd->capa, NULL);
      switch (status)
	{
	case 0:
	  break;

	case MU_ERR_NOENT:
	  return ENOSYS;

	default:
	  return status;
	}
    }
  
  if (imap->client_state == MU_IMAP_CLIENT_READY)
    {
      status = _mu_imap_tag_next (imap);
      MU_IMAP_CHECK_EAGAIN (imap, status);
      status = mu_imapio_send_command_v (imap->io, imap->tag_str,
					 cmd->msgset,
					 cmd->argc, cmd->argv, cmd->extra);
      MU_IMAP_CHECK_ERROR (imap, status);
      MU_IMAP_FCLR (imap, MU_IMAP_RESP);
      imap->client_state = cmd->rx_state;
    }

  if (imap->client_state == cmd->rx_state)
    {
      status = _mu_imap_response (imap, cmd->untagged_handler,
				  cmd->untagged_handler_data);
      MU_IMAP_CHECK_EAGAIN (imap, status);
      if (cmd->tagged_handler)
	  cmd->tagged_handler (imap);
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
    }
  else
    status = EINPROGRESS;

  return status;
}
      
