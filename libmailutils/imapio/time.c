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
#include <stdlib.h>
#include <mailutils/stream.h>
#include <mailutils/errno.h>
#include <mailutils/sys/imapio.h>
#include <mailutils/datetime.h>

int
mu_imapio_send_time (struct _mu_imapio *io, struct tm *tm,
		     struct mu_timezone *tz)
{
  return mu_c_streamftime (io->_imap_stream,
			   "\"" MU_DATETIME_INTERNALDATE "\"", tm, tz);
}  
