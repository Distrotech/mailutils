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
#include <mailutils/errno.h>
#include <mailutils/imapio.h>
#include <mailutils/sys/imapio.h>

int
mu_imapio_get_words (mu_imapio_t io, size_t *pwc, char ***pwv)
{
  if (!io->_imap_reply_ready)
    return MU_ERR_INFO_UNAVAILABLE;
  *pwc = io->_imap_ws.ws_wordc;
  *pwv = io->_imap_ws.ws_wordv;
  return 0;
}

int
mu_imapio_getbuf (mu_imapio_t io, char **pptr, size_t *psize)
{
  if (!io->_imap_reply_ready)
    return MU_ERR_INFO_UNAVAILABLE;
  *pptr  = io->_imap_buf_base;
  *psize = io->_imap_buf_level;
  return 0;
}
  
