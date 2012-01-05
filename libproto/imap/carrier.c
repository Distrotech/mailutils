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

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <mailutils/errno.h>
#include <mailutils/imapio.h>
#include <mailutils/sys/imap.h>
#include <mailutils/sys/imapio.h>

int
mu_imap_set_carrier (mu_imap_t imap, mu_stream_t carrier)
{
  int rc;
  mu_imapio_t io;
  
  /* Sanity checks.  */
  if (imap == NULL)
    return EINVAL;

  rc = mu_imapio_create (&io, carrier, MU_IMAPIO_CLIENT);
  if (rc)
    return rc;
  if (imap->io)
    {
      /* Close any old carrier.  */
      mu_imap_disconnect (imap);
      mu_imapio_free (imap->io);
    }
  imap->io = io;
  if (MU_IMAP_FISSET (imap, MU_IMAP_TRACE))
    _mu_imap_trace_enable (imap);
  imap->client_state = MU_IMAP_CLIENT_READY;
  imap->session_state = MU_IMAP_SESSION_INIT;
  return 0;
}

int
mu_imap_get_carrier (mu_imap_t imap, mu_stream_t *pcarrier)
{
  /* Sanity checks.  */
  if (imap == NULL)
    return EINVAL;
  if (pcarrier == NULL)
    return MU_ERR_OUT_PTR_NULL;

  mu_stream_ref (imap->io->_imap_stream);
  *pcarrier = imap->io->_imap_stream;
  return 0;
}

