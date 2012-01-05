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
#include <mailutils/types.h>
#include <mailutils/imapio.h>
#include <mailutils/errno.h>
#include <mailutils/argcv.h>
#include <mailutils/sys/imapio.h>

/* FIXME: 1. Is it needed at all?
   2. It would be good to have a `, size_t *psize' argument, to allow
   for passing an already allocated buffer.  This implies similar changes
   to mu_argcv_join. */
int
mu_imapio_reply_string (struct _mu_imapio *io, size_t start, char **pbuf)
{
  if (!io->_imap_reply_ready)
    return MU_ERR_NOENT;

  if (start >= io->_imap_ws.ws_wordc)
    return EINVAL;
  
  return mu_argcv_join (io->_imap_ws.ws_wordc - start,
			io->_imap_ws.ws_wordv + start, " ",
			mu_argcv_escape_no,
			pbuf);
}
