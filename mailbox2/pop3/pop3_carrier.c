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

#include <string.h>
#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif

#include <stdlib.h>
#include <mailutils/sys/pop3.h>

int
pop3_set_carrier (pop3_t pop3, stream_t carrier)
{
  /* Sanity checks.  */
  if (pop3 == NULL)
    return MU_ERROR_INVALID_PARAMETER;

  if (pop3->carrier)
    {
      stream_close (pop3->carrier);
      stream_destroy (&pop3->carrier);
    }
  pop3->carrier = carrier;
  return 0;
}

int
pop3_get_carrier (pop3_t pop3, stream_t *pcarrier)
{
  /* Sanity checks.  */
  if (pop3 == NULL || pcarrier == NULL)
    return MU_ERROR_INVALID_PARAMETER;

  if (pop3->carrier == NULL)
    {
      stream_t carrier = NULL;
      int status = stream_tcp_create (&carrier);
      if (status != 0)
	return status;
      status = stream_buffer_create (&(pop3->carrier), carrier, 1024);
      if (status != 0)
	{
	  stream_destroy (&carrier);
	  return status;
	}
    }
  /* Since we expose the stream incremente the reference count.  */
  stream_ref (pop3->carrier);
  *pcarrier = pop3->carrier;
  return 0;
}
