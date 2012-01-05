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
#include <errno.h>
#include <mailutils/errno.h>
#include <mailutils/sys/imap.h>
#include <mailutils/list.h>

int
mu_imap_create (mu_imap_t *pimap)
{
  mu_imap_t imap;

  /* Sanity check.  */
  if (pimap == NULL)
    return EINVAL;

  imap = calloc (1, sizeof *imap);
  if (imap == NULL)
    return ENOMEM;

  _mu_imap_init (imap);

  *pimap = imap;
  return 0;
}

int
_mu_imap_init (mu_imap_t imap)
{
  if (imap == NULL)
    return EINVAL;
  if (!imap->io)
    {
      int rc;
      
      mu_list_destroy (&imap->capa);
      _mu_imap_clrerrstr (imap);
      rc = _mu_imap_tag_clr (imap);
      imap->flags = 0;
      if (rc)
	return rc;
    }
  imap->client_state = MU_IMAP_CLIENT_READY; 
  imap->session_state = MU_IMAP_SESSION_INIT;
  return 0;
}

