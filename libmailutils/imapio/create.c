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
#include <mailutils/stream.h>
#include <mailutils/errno.h>
#include <mailutils/sys/imapio.h>

int
mu_imapio_create (mu_imapio_t *iop, mu_stream_t str, int server)
{
  struct _mu_imapio *io = calloc (1, sizeof (*io));
  if (!io)
    return ENOMEM;
  io->_imap_stream = str;
  mu_stream_ref (str);
  io->_imap_ws.ws_delim = " \t()[]";
  io->_imap_ws.ws_escape = "\\\"";
  io->_imap_ws_flags = MU_WRDSF_DELIM |
                       MU_WRDSF_ESCAPE |
                       MU_WRDSF_NOVAR |
                       MU_WRDSF_NOCMD | 
                       MU_WRDSF_DQUOTE |
                       MU_WRDSF_RETURN_DELIMS |
                       MU_WRDSF_WS |
                       MU_WRDSF_APPEND;
  io->_imap_server = server;
  *iop = io;
  return 0;
}

void
mu_imapio_free (mu_imapio_t io)
{
  if (!io)
    return;
  if (io->_imap_ws_flags & MU_WRDSF_REUSE)
    mu_wordsplit_free (&io->_imap_ws);
  mu_stream_unref (io->_imap_stream);
  free (io);
}

void
mu_imapio_destroy (mu_imapio_t *pio)
{
  if (!pio)
    return;
  mu_imapio_free (*pio);
  *pio = NULL;
}

