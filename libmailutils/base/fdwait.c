/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2003, 2005-2007, 2009-2012 Free Software
   Foundation, Inc.

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

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <mailutils/stream.h>

int
mu_fd_wait (int fd, int *pflags, struct timeval *tvp)
{
  fd_set rdset, wrset, exset;
  int rc;

  FD_ZERO (&rdset);
  FD_ZERO (&wrset);
  FD_ZERO (&exset);
  if ((*pflags) & MU_STREAM_READY_RD)
    FD_SET (fd, &rdset);
  if ((*pflags) & MU_STREAM_READY_WR)
    FD_SET (fd, &wrset);
  if ((*pflags) & MU_STREAM_READY_EX)
    FD_SET (fd, &exset);
  
  do
    {
      if (tvp)
	{
	  struct timeval tv = *tvp; 
	  rc = select (fd + 1, &rdset, &wrset, &exset, &tv);
	}
      else
	rc = select (fd + 1, &rdset, &wrset, &exset, NULL);
    }
  while (rc == -1 && errno == EINTR);

  if (rc < 0)
    return errno;
  
  *pflags = 0;
  if (rc > 0)
    {
      if (FD_ISSET (fd, &rdset))
	*pflags |= MU_STREAM_READY_RD;
      if (FD_ISSET (fd, &wrset))
	*pflags |= MU_STREAM_READY_WR;
      if (FD_ISSET (fd, &exset))
	*pflags |= MU_STREAM_READY_EX;
    }
  return 0;
}
