/* GNU Mailutils -- a suite of utilities for electronic mail
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

#include <stdlib.h>
#include <mailutils/types.h>
#include <mailutils/stream.h>
#include <mailutils/errno.h>
#include <mailutils/imap.h>
#include <mailutils/sys/imap.h>

int
mu_imap_append_stream (mu_imap_t imap, const char *mailbox, int flags,
		       struct tm *tm, struct mu_timezone *tz,
		       mu_stream_t stream)
{
  mu_off_t size;
  int rc;

  rc = mu_stream_size (stream, &size);
  if (rc == 0)
    rc = mu_imap_append_stream_size (imap, mailbox, flags, tm, tz, stream,
				     size);
  return rc;
}

      
      

