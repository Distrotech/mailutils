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

#include <stdlib.h>
#include <mailutils/error.h>
#include <mailutils/sys/pop3.h>

/* Initialise a pop3_t handle.  */
int
pop3_create (pop3_t *ppop3)
{
  pop3_t pop3;

  if (ppop3 == NULL)
    return MU_ERROR_INVALID_PARAMETER;

  pop3 = calloc (1, sizeof *pop3);
  if (pop3 == NULL)
    return MU_ERROR_NO_MEMORY;

  /* According to RFC 2449: The maximum length of the first line of a
     command response (including the initial greeting) is unchanged at
     512 octets (including the terminating CRLF).  */
  pop3->ack.len = 512;
  pop3->ack.ptr = pop3->ack.buf = calloc (pop3->ack.len, 1);
  if (pop3->ack.buf == NULL)
    {
      pop3_destroy (pop3);
      return MU_ERROR_NO_MEMORY;
    }

  /* RFC 2449 recommands 255, but we grow it as needed.  */
  pop3->io.len = 255;
  pop3->io.ptr = pop3->io.buf = calloc (pop3->io.len, 1);
  if (pop3->io.buf == NULL)
    {
      pop3_destroy (pop3);
      return MU_ERROR_NO_MEMORY;
    }

  pop3->state = POP3_NO_STATE; /* Init with no state.  */
  pop3->timeout = 10 * 60; /* The default Timeout is 10 minutes.  */
  pop3->acknowledge = 0; /* No Ack received.  */

  *ppop3 = pop3;
  return 0; /* Okdoke.  */
}
