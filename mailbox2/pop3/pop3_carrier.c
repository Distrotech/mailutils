/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Library Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

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
  *pcarrier = pop3->carrier;
  return 0;
}
