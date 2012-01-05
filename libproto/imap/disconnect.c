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
#include <errno.h>
#include <mailutils/imapio.h>
#include <mailutils/sys/imap.h>

int
mu_imap_disconnect (mu_imap_t imap)
{
  /* Sanity checks.  */
  if (imap == NULL)
    return EINVAL;

  imap->client_state = MU_IMAP_CLIENT_READY;
  MU_IMAP_FCLR (imap, MU_IMAP_RESP);

  mu_list_clear (imap->capa);
  
  /* Close the stream.  */
  mu_imapio_destroy (&imap->io);

  return 0;
}
