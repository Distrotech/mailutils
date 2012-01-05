/* Response processing for IMAP client.
   GNU Mailutils -- a suite of utilities for electronic mail
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
#include <mailutils/errno.h>
#include <mailutils/sys/imap.h>

void
mu_imap_callback (mu_imap_t imap, int code, size_t sdat, void *pdat)
{
  if (code < 0 || code >= _MU_IMAP_CB_MAX || !imap->callback[code].action)
    return;
  imap->callback[code].action (imap->callback[code].data, code, sdat, pdat);
}

void
mu_imap_register_callback_function (mu_imap_t imap, int code,
				    mu_imap_callback_t callback,
				    void *data)
{
  if (code < 0 || code >= _MU_IMAP_CB_MAX)
    {
      mu_error ("%s:%d: ignoring unsupported callback code %d",
		__FILE__, __LINE__, code);
      return;
    }
  imap->callback[code].action = callback;
  imap->callback[code].data = data;
}
