/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2003, 2007, 2010 Free Software
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
   Public License along with this library; if not, write to the
   Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301 USA */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <mailutils/pop3.h>
#include <mailutils/sys/pop3.h>
#include <mailutils/tls.h>
#include <mailutils/list.h>

static int
pop3_swap_streams (mu_pop3_t pop3, mu_stream_t *streams)
{
  int rc;
  
  if (MU_POP3_FISSET (pop3, MU_POP3_TRACE))
    rc = mu_stream_ioctl (pop3->carrier, MU_IOCTL_SWAP_STREAM, streams);
  else if (streams[0] != streams[1])
    rc = EINVAL;
  else
    {
      mu_stream_t str = streams[0];
      streams[0] = streams[1] = pop3->carrier;
      pop3->carrier = str;
      rc = 0;
    }
  return rc;
}

/*
 * STLS
 * We have to assume that the caller check the CAPA and TLS was supported.
 */
int
mu_pop3_stls (mu_pop3_t pop3)
{
#ifdef WITH_TLS
  int status;
  mu_stream_t tlsstream, streams[2];
  
  /* Sanity checks.  */
  if (pop3 == NULL)
    return EINVAL;

  switch (pop3->state)
    {
    case MU_POP3_NO_STATE:
      status = mu_pop3_writeline (pop3, "STLS\r\n");
      MU_POP3_CHECK_ERROR (pop3, status);
      MU_POP3_FCLR (pop3, MU_POP3_ACK);
      pop3->state = MU_POP3_STLS;

    case MU_POP3_STLS:
      status = mu_pop3_response (pop3, NULL);
      MU_POP3_CHECK_EAGAIN (pop3, status);
      MU_POP3_CHECK_OK (pop3);
      pop3->state = MU_POP3_STLS_CONNECT;
      
    case MU_POP3_STLS_CONNECT:
      streams[0] = streams[1] = NULL;
      status = pop3_swap_streams (pop3, streams);
      MU_POP3_CHECK_EAGAIN (pop3, status);
      status = mu_tls_client_stream_create (&tlsstream,
					    streams[0], streams[1], 0);
      MU_POP3_CHECK_EAGAIN (pop3, status);
      status = mu_stream_open (tlsstream);
      MU_POP3_CHECK_EAGAIN (pop3, status);
      streams[0] = streams[1] = tlsstream;
      status = pop3_swap_streams (pop3, streams);
      MU_POP3_CHECK_EAGAIN (pop3, status);
      /* Invalidate the capability list */
      mu_list_destroy (&pop3->capa);
      pop3->state = MU_POP3_NO_STATE;
      return 0;
      
      /* They must deal with the error first by reopening.  */
    case MU_POP3_ERROR:
      status = ECANCELED;
      break;

      /* No case in the switch another operation was in progress.  */
    default:
      status = EINPROGRESS;
    }

  return status;
#else
  return ENOTSUP;
#endif
}
