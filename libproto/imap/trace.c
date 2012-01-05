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

#include <mailutils/errno.h>
#include <mailutils/imapio.h>
#include <mailutils/sys/imap.h>

int
_mu_imap_trace_enable (mu_imap_t imap)
{
  int rc;
  if (!imap->io)
    return 0;
  rc = mu_imapio_trace_enable (imap->io);
  switch (rc)
    {
    case 0:
    case MU_ERR_OPEN:
      MU_IMAP_FSET (imap, MU_IMAP_TRACE);
      break;
    }
  return rc;
}

int
_mu_imap_trace_disable (mu_imap_t imap)
{
  int rc;
  if (!imap->io)
    return 0;
  rc = mu_imapio_trace_disable (imap->io);
  switch (rc)
    {
    case 0:
    case MU_ERR_NOT_OPEN:
      MU_IMAP_FCLR (imap, MU_IMAP_TRACE);
      break;
    }
  return rc;

}

int
mu_imap_trace (mu_imap_t imap, int op)
{
  int trace_on = mu_imapio_get_trace (imap->io);

  switch (op)
    {
    case MU_IMAP_TRACE_SET:
      if (trace_on)
	return MU_ERR_EXISTS;
      return _mu_imap_trace_enable (imap);
      
    case MU_IMAP_TRACE_CLR:
      if (!trace_on)
	return MU_ERR_NOENT;
      return _mu_imap_trace_disable (imap);

    case MU_IMAP_TRACE_QRY:
      if (!trace_on)
	return MU_ERR_NOENT;
      return 0;
    }
  return EINVAL;
}

int
mu_imap_trace_mask (mu_imap_t imap, int op, int lev)
{
  switch (op)
    {
    case MU_IMAP_TRACE_SET:
      MU_IMAP_SET_XSCRIPT_MASK (imap, lev);
      if (lev & MU_XSCRIPT_PAYLOAD)
	mu_imapio_trace_payload (imap->io, 1);
      break;
      
    case MU_IMAP_TRACE_CLR:
      MU_IMAP_CLR_XSCRIPT_MASK (imap, lev);
      if (lev & MU_XSCRIPT_PAYLOAD)
	mu_imapio_trace_payload (imap->io, 0);
      break;
      
    case MU_IMAP_TRACE_QRY:
      if (MU_IMAP_IS_XSCRIPT_MASK (imap, lev))
	break;
      return MU_ERR_NOENT;
      
    default:
      return EINVAL;
    }
  
  return 0;
}

int
_mu_imap_xscript_level (mu_imap_t imap, int xlev)
{
  return mu_imapio_set_xscript_level (imap->io, xlev);
}
