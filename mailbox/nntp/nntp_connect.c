/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2004 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <mailutils/sys/nntp.h>

/* Open the connection to the server. */
int
mu_nntp_connect (mu_nntp_t nntp)
{
  int status;

  /* Sanity checks.  */
  if (nntp == NULL)
    return EINVAL;

  /* A networking stack.  */
  if (nntp->carrier == NULL)
    return EINVAL;

  /* Enter the pop state machine, and boogy: AUTHORISATION State.  */
  switch (nntp->state)
    {
    default:
      /* FALLTHROUGH */
      /* If nntp was in an error state going through here should clear it.  */

    case MU_NNTP_NO_STATE:
      status = mu_nntp_disconnect (nntp);
      MU_NNTP_CHECK_EAGAIN (nntp, status);
      nntp->state = MU_NNTP_CONNECT;

    case MU_NNTP_CONNECT:
      /* Establish the connection.  */
      status = stream_open (nntp->carrier);
      MU_NNTP_CHECK_EAGAIN (nntp, status);
      nntp->acknowledge = 0;
      nntp->state = MU_NNTP_GREETINGS;

    case MU_NNTP_GREETINGS:
      /* Get the greetings.  */
      {
	size_t len = 0;
	char *right, *left;
	status = mu_nntp_response (nntp, NULL, 0, &len);
	MU_NNTP_CHECK_EAGAIN (nntp, status);
	mu_nntp_debug_ack (nntp);
	if (nntp->ack.buf[0] == '2')
	  {
	    stream_close (nntp->carrier);
	    nntp->state = MU_NNTP_NO_STATE;
	    return EACCES;
	  }
	nntp->state = MU_NNTP_NO_STATE;
      }
    } /* End AUTHORISATION state. */

  return status;
}
