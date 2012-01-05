/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003, 2007, 2010-2012 Free Software Foundation, Inc.

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
#include <errno.h>
#include <mailutils/errno.h>
#include <mailutils/sys/pop3.h>
#include <mailutils/list.h>

/* Initialise a mu_pop3_t handle.  */

int
mu_pop3_create (mu_pop3_t *ppop3)
{
  mu_pop3_t pop3;

  /* Sanity check.  */
  if (ppop3 == NULL)
    return EINVAL;

  pop3 = calloc (1, sizeof *pop3);
  if (pop3 == NULL)
    return ENOMEM;

  pop3->state = MU_POP3_NO_STATE; /* Init with no state.  */
  pop3->timeout = (10 * 60) * 100; /* The default Timeout is 10 minutes.  */
  MU_POP3_FCLR (pop3, MU_POP3_ACK); /* No Ack received.  */

  *ppop3 = pop3;
  return 0; /* Okdoke.  */
}

int
_mu_pop3_init (mu_pop3_t pop3)
{
  if (pop3 == NULL)
    return EINVAL;
  if (pop3->carrier == 0)
    {
      mu_list_destroy (&pop3->capa);
      pop3->flags = 0;
    }
  return 0;
}

  
