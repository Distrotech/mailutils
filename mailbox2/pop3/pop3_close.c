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

#include <mailutils/sys/pop3.h>
#include <stdlib.h>
#include <string.h>

/* Sudden death.  */
int
pop3_close (pop3_t pop3)
{
  /* Sanity checks.  */
  if (pop3 == NULL)
    return MU_ERROR_INVALID_PARAMETER;

  /* We can keep some of the fields, if they decide to pop3_open() again but
     clear the states.  */
  pop3->state = POP3_NO_STATE;
  pop3->acknowledge = 0;
  memset (pop3->io.buf, '\0', pop3->io.len);
  memset (pop3->ack.buf, '\0', pop3->ack.len);

  /* Free the timestamp, it will be different on each connection.  */
  if (pop3->timestamp)
    {
      free (pop3->timestamp);
      pop3->timestamp = NULL;
    }
  return stream_close (pop3->stream);
}
