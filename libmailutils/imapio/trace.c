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
#include <errno.h>
#include <mailutils/types.h>
#include <mailutils/imapio.h>
#include <mailutils/stream.h>
#include <mailutils/errno.h>
#include <mailutils/error.h>
#include <mailutils/nls.h>
#include <mailutils/sys/imapio.h>

static const char *imapio_prefix[] = {
  "S: ", "C: "
};

int
mu_imapio_trace_enable (mu_imapio_t io)
{
  int rc = 0;
  mu_stream_t dstr, xstr;

  if (io->_imap_transcript)
    return MU_ERR_OPEN;
  
  rc = mu_dbgstream_create (&dstr, MU_DIAG_DEBUG);
  if (rc)
    mu_error (_("cannot create debug stream; transcript disabled: %s"),
	      mu_strerror (rc));
  else
    {
      rc = mu_xscript_stream_create (&xstr, io->_imap_stream, dstr,
				     imapio_prefix);
      if (rc)
	mu_error (_("cannot create transcript stream: %s"),
		  mu_strerror (rc));
      else
	{
	  mu_stream_unref (io->_imap_stream);
	  io->_imap_stream = xstr;
	  io->_imap_transcript = 1;
	}
    }

  return rc;
}

int
mu_imapio_trace_disable (mu_imapio_t io)
{
  mu_stream_t xstr = io->_imap_stream;
  mu_stream_t stream[2];
  int rc;

  if (!io->_imap_transcript)
    return MU_ERR_NOT_OPEN;
  
  rc = mu_stream_ioctl (xstr, MU_IOCTL_TRANSPORT, MU_IOCTL_OP_GET, stream);
  if (rc)
    return rc;

  io->_imap_stream = stream[0];
  mu_stream_destroy (&xstr);
  io->_imap_transcript = 0;
  return 0;
}

int
mu_imapio_get_trace (mu_imapio_t io)
{
  return io ? io->_imap_transcript : 0; 
}

void
mu_imapio_trace_payload (mu_imapio_t io, int val)
{
  io->_imap_trace_payload = !!val;
}

int
mu_imapio_get_trace_payload (mu_imapio_t io)
{
  return io ? io->_imap_trace_payload : 0;
}
