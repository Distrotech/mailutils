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
	stream_destroy (&nntp->carrier, nntp);

      free (nntp);

      *pnntp = NULL;
    }
}
