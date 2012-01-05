/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2001, 2007, 2009-2012 Free Software Foundation,
   Inc.

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
#include <mailutils/nls.h>
#include <mailutils/stream.h>
#include <mailutils/sys/pop3.h>

static const char *pop3_prefix[] = {
  "S: ", "C: "
};

int
_mu_pop3_trace_enable (mu_pop3_t pop3)
{
  int rc = 0;
  mu_stream_t dstr, xstr;

  if (!pop3->carrier)
    {
      MU_POP3_FSET (pop3, MU_POP3_TRACE);
      return 0;
    }
  
  rc = mu_dbgstream_create (&dstr, MU_DIAG_DEBUG);
  if (rc)
    mu_error (_("cannot create debug stream; transcript disabled: %s"),
	      mu_strerror (rc));
  else
    {
      rc = mu_xscript_stream_create (&xstr, pop3->carrier, dstr,
				     pop3_prefix);
      if (rc)
	mu_error (_("cannot create transcript stream: %s"),
		  mu_strerror (rc));
      else
	{
	  mu_stream_unref (pop3->carrier);
	  pop3->carrier = xstr;
	  MU_POP3_FSET (pop3, MU_POP3_TRACE);
	}
    }

  return rc;
}

int
_mu_pop3_trace_disable (mu_pop3_t pop3)
{
  mu_stream_t xstr = pop3->carrier;
  mu_stream_t stream[2];
  int rc;

  if (!xstr)
    return 0;
  
  rc = mu_stream_ioctl (xstr, MU_IOCTL_TRANSPORT, MU_IOCTL_OP_GET, stream);
  if (rc)
    return rc;

  pop3->carrier = stream[0];
  mu_stream_destroy (&xstr);
  MU_POP3_FCLR (pop3, MU_POP3_TRACE);
  return 0;
}

int
mu_pop3_trace (mu_pop3_t pop3, int op)
{
  int trace_on = MU_POP3_FISSET (pop3, MU_POP3_TRACE);
  
  switch (op)
    {
    case MU_POP3_TRACE_SET:
      if (trace_on)
	return MU_ERR_EXISTS;
      return _mu_pop3_trace_enable (pop3);
      
    case MU_POP3_TRACE_CLR:
      if (!trace_on)
	return MU_ERR_NOENT;
      return _mu_pop3_trace_disable (pop3);

    case MU_POP3_TRACE_QRY:
      if (!trace_on)
	return MU_ERR_NOENT;
      return 0;
    }
  return EINVAL;
}

int
mu_pop3_trace_mask (mu_pop3_t pop3, int op, int lev)
{
  switch (op)
    {
    case MU_POP3_TRACE_SET:
      pop3->flags |= MU_POP3_XSCRIPT_MASK(lev);
      break;
      
    case MU_POP3_TRACE_CLR:
      pop3->flags &= ~MU_POP3_XSCRIPT_MASK(lev);
      break;
      
    case MU_POP3_TRACE_QRY:
      if (pop3->flags & MU_POP3_XSCRIPT_MASK(lev))
	break;
      return MU_ERR_NOENT;
      
    default:
      return EINVAL;
    }
  return 0;
}

int
_mu_pop3_xscript_level (mu_pop3_t pop3, int xlev)
{
  if (mu_stream_ioctl (pop3->carrier, MU_IOCTL_XSCRIPTSTREAM,
                       MU_IOCTL_XSCRIPTSTREAM_LEVEL, &xlev) == 0)
    return xlev;
  return MU_XSCRIPT_NORMAL;
}

  
