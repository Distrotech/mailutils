/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

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

#include <mailutils/error.h>
#include <mailutils/sys/pop3.h>
#include <stdlib.h>

void
pop3_destroy (pop3_t *ppop3)
{
  if (ppop3 && *ppop3)
    {
      pop3_t pop3 = *ppop3;
      if (pop3->ack.buf)
	free (pop3->ack.buf);

      if (pop3->io.buf)
	free (pop3->io.buf);

      if (pop3->timestamp)
	free (pop3->timestamp);

      if (pop3->carrier)
	stream_destroy (&pop3->carrier);

      if (pop3->debug)
	mu_debug_destroy (&pop3->debug);

      free (pop3);
      *ppop3 = NULL;
    }
}
