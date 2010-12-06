/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2010 Free Software Foundation, Inc.

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
#include <errno.h>
#include <stdio.h>
#include <mailutils/error.h>
#include <mailutils/errno.h>
#include <mailutils/nls.h>
#include <mailutils/stream.h>
#include <mailutils/sys/imap.h>

static const char *imap_prefix[] = {
  "S: ", "C: "
};

int
_mu_imap_trace_enable (mu_imap_t imap)
{
  int rc = 0;
  mu_stream_t dstr, xstr;

  if (!imap->carrier)
    {
      MU_IMAP_FSET (imap, MU_IMAP_TRACE);
      return 0;
    }
  
  rc = mu_dbgstream_create (&dstr, MU_DIAG_DEBUG);
  if (rc)
    mu_error (_("cannot create debug stream; transcript disabled: %s"),
	      mu_strerror (rc));
  else
    {
      rc = mu_xscript_stream_create (&xstr, imap->carrier, dstr,
				     imap_prefix);
      if (rc)
	mu_error (_("cannot create transcript stream: %s"),
		  mu_strerror (rc));
      else
	{
	  mu_stream_unref (imap->carrier);
	  imap->carrier = xstr;
	  MU_IMAP_FSET (imap, MU_IMAP_TRACE);
	}
    }

  return rc;
}

int
_mu_imap_trace_disable (mu_imap_t imap)
{
  mu_stream_t xstr = imap->carrier;
  mu_stream_t stream[2];
  int rc;

  if (!xstr)
    return 0;
  
  rc = mu_stream_ioctl (xstr, MU_IOCTL_TRANSPORT, MU_IOCTL_OP_GET, stream);
  if (rc)
    return rc;

  imap->carrier = stream[0];
  mu_stream_destroy (&xstr);
  MU_IMAP_FCLR (imap, MU_IMAP_TRACE);
  return 0;
}

int
mu_imap_trace (mu_imap_t imap, int op)
{
  int trace_on = MU_IMAP_FISSET (imap, MU_IMAP_TRACE);
  
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
      imap->flags |= MU_IMAP_XSCRIPT_MASK(lev);
      break;
      
    case MU_IMAP_TRACE_CLR:
      imap->flags &= ~MU_IMAP_XSCRIPT_MASK(lev);
      break;
      
    case MU_IMAP_TRACE_QRY:
      if (imap->flags & MU_IMAP_XSCRIPT_MASK(lev))
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
  if (mu_stream_ioctl (imap->carrier, MU_IOCTL_XSCRIPTSTREAM,
                       MU_IOCTL_XSCRIPTSTREAM_LEVEL, &xlev) == 0)
    return xlev;
  return MU_XSCRIPT_NORMAL;
}
