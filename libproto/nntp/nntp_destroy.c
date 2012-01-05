/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2004, 2007, 2010-2012 Free Software Foundation, Inc.

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
#include <mailutils/errno.h>
#include <mailutils/sys/nntp.h>

void
mu_nntp_destroy (mu_nntp_t *pnntp)
{
  if (pnntp && *pnntp)
    {
      mu_nntp_t nntp = *pnntp;

      /* Free the response buffer.  */
      if (nntp->ack.buf)
	free (nntp->ack.buf);

      /* Free the io buffer.  */
      if (nntp->io.buf)
	free (nntp->io.buf);

      /* Release the carrier.  */
      if (nntp->carrier)
	mu_stream_destroy (&nntp->carrier);

      /* Any posting residue.  */
      if (nntp->post.buf)
	free (nntp->post.buf);
      free (nntp);

      *pnntp = NULL;
    }
}
