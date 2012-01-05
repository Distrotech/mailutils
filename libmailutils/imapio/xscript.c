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
mu_imapio_set_xscript_level (struct _mu_imapio *io, int xlev)
{
  if (!io)
    return EINVAL;
  if (io->_imap_transcript)
    {
      if (xlev != MU_XSCRIPT_NORMAL)
	{
	  if (mu_debug_level_p (MU_DEBCAT_REMOTE, 
	                        xlev == MU_XSCRIPT_SECURE ?
				  MU_DEBUG_TRACE6 : MU_DEBUG_TRACE7))
	    return MU_XSCRIPT_NORMAL;
	}

      if (mu_stream_ioctl (io->_imap_stream, MU_IOCTL_XSCRIPTSTREAM,
                           MU_IOCTL_XSCRIPTSTREAM_LEVEL, &xlev) == 0)
	return xlev;
    }
  return MU_XSCRIPT_NORMAL;
}
