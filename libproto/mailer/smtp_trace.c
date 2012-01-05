/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2010-2012 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>. */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <mailutils/errno.h>
#include <mailutils/error.h>
#include <mailutils/nls.h>
#include <mailutils/stream.h>
#include <mailutils/sys/smtp.h>

static const char *smtp_prefix[] = {
  "S: ", "C: "
};

int
_mu_smtp_trace_enable (mu_smtp_t smtp)
{
  int rc = 0;
  mu_stream_t dstr, xstr;

  if (!smtp->carrier)
    {
      MU_SMTP_FSET (smtp, _MU_SMTP_TRACE);
      return 0;
    }
  
  rc = mu_dbgstream_create (&dstr, MU_DIAG_DEBUG);
  if (rc)
    mu_error (_("cannot create debug stream; transcript disabled: %s"),
	      mu_strerror (rc));
  else
    {
      rc = mu_xscript_stream_create (&xstr, smtp->carrier, dstr,
				     smtp_prefix);
      if (rc)
	mu_error (_("cannot create transcript stream: %s"),
		  mu_strerror (rc));
      else
	{
	  mu_stream_unref (smtp->carrier);
	  smtp->carrier = xstr;
	  MU_SMTP_FSET (smtp, _MU_SMTP_TRACE);
	}
    }

  return rc;
}

int
_mu_smtp_trace_disable (mu_smtp_t smtp)
{
  mu_stream_t xstr = smtp->carrier;
  mu_stream_t stream[2];
  int rc;

  if (!xstr)
    return 0;
  
  rc = mu_stream_ioctl (xstr, MU_IOCTL_TRANSPORT, MU_IOCTL_OP_GET, stream);
  if (rc)
    return rc;

  smtp->carrier = stream[0];
  mu_stream_destroy (&xstr);
  MU_SMTP_FCLR (smtp, _MU_SMTP_TRACE);
  return 0;
}

int
mu_smtp_trace (mu_smtp_t smtp, int op)
{
  int trace_on = MU_SMTP_FISSET (smtp, _MU_SMTP_TRACE);
  
  switch (op)
    {
    case MU_SMTP_TRACE_SET:
      if (trace_on)
	return MU_ERR_EXISTS;
      return _mu_smtp_trace_enable (smtp);
      
    case MU_SMTP_TRACE_CLR:
      if (!trace_on)
	return MU_ERR_NOENT;
      return _mu_smtp_trace_disable (smtp);

    case MU_SMTP_TRACE_QRY:
      if (!trace_on)
	return MU_ERR_NOENT;
      return 0;
    }
  return EINVAL;
}

int
mu_smtp_trace_mask (mu_smtp_t smtp, int op, int lev)
{
  switch (op)
    {
    case MU_SMTP_TRACE_SET:
      smtp->flags |= MU_SMTP_XSCRIPT_MASK (lev);
      break;
      
    case MU_SMTP_TRACE_CLR:
      smtp->flags &= ~MU_SMTP_XSCRIPT_MASK (lev);
      break;
      
    case MU_SMTP_TRACE_QRY:
      if (smtp->flags & MU_SMTP_XSCRIPT_MASK (lev))
	break;
      return MU_ERR_NOENT;
      
    default:
      return EINVAL;
    }
  return 0;
}

int
_mu_smtp_xscript_level (mu_smtp_t smtp, int xlev)
{
  if (mu_stream_ioctl (smtp->carrier, MU_IOCTL_XSCRIPTSTREAM,
                       MU_IOCTL_XSCRIPTSTREAM_LEVEL, &xlev) == 0)
    return xlev;
  return MU_XSCRIPT_NORMAL;
}
