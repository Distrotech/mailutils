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

#include <stdlib.h>

#include <mailutils/error.h>
#include <mailutils/sys/ticket.h>

int
ticket_ref (ticket_t ticket)
{
  if (ticket == NULL || ticket->vtable == NULL
      || ticket->vtable->ref == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return ticket->vtable->ref (ticket);
}

void
ticket_destroy (ticket_t *pticket)
{
  if (pticket && *pticket)
    {
      ticket_t ticket = *pticket;
      if (ticket->vtable && ticket->vtable->destroy)
	ticket->vtable->destroy (pticket);
      *pticket = NULL;
    }
}

int
ticket_pop (ticket_t ticket, const char *challenge, char **parg)
{
  if (ticket == NULL || ticket->vtable == NULL
      || ticket->vtable->pop == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return ticket->vtable->pop (ticket, challenge, parg);
}
