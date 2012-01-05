/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001-2012 Free Software Foundation, Inc.

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
#include <mailutils/stream.h>
#include <mailutils/diag.h>
#include <mailutils/imapio.h>
#include <mailutils/sys/imapio.h>

int
mu_imapio_get_streams (struct _mu_imapio *io, mu_stream_t *streams)
{
  int rc;

  mu_stream_flush (io->_imap_stream);
  if (io->_imap_transcript)
    rc = mu_stream_ioctl (io->_imap_stream, MU_IOCTL_SUBSTREAM, 
			  MU_IOCTL_OP_GET, streams);
  else
    {
      streams[0] = io->_imap_stream;
      mu_stream_ref (streams[0]);
      streams[1] = io->_imap_stream;
      mu_stream_ref (streams[1]);
      rc = 0;
    }
  return rc;
}
      
int
mu_imapio_set_streams (struct _mu_imapio *io, mu_stream_t *streams)
{
  int rc;
  
  if (io->_imap_transcript)
    rc = mu_stream_ioctl (io->_imap_stream, MU_IOCTL_SUBSTREAM, 
                          MU_IOCTL_OP_SET, streams);
  else
    {
      mu_stream_t tmp;
      
      if (streams[0] == streams[1])
	{
	  tmp = streams[0];
	  mu_stream_ref (tmp);
	  mu_stream_ref (tmp);
	  rc = 0;
	}
      else
	rc = mu_iostream_create (&tmp, streams[0], streams[1]);
      if (rc == 0)
	{
	  mu_stream_unref (io->_imap_stream);
	  io->_imap_stream = tmp;
	}
    }
  return rc;
}
