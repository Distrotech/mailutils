/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2011-2012 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>. */

#include <config.h>
#include <stdlib.h>
#include <string.h>
#include <mailutils/types.h>
#include <mailutils/errno.h>
#include <mailutils/imapio.h>
#include <mailutils/sys/imapio.h>
#include <mailutils/stream.h>

int
mu_imapio_send_literal_string (struct _mu_imapio *io, const char *buffer)
{
  size_t len = strlen (buffer);

  mu_stream_printf (io->_imap_stream, "{%lu}\r\n", (unsigned long) len);

  if (!io->_imap_server)
    {
      int rc = mu_imapio_getline (io);
      if (rc)
	return rc;
      if (!(io->_imap_reply_ready && io->_imap_ws.ws_wordv[0][0] == '+'))
	return MU_ERR_BADREPLY;
    }

  return mu_stream_write (io->_imap_stream, buffer, len, NULL);
}
