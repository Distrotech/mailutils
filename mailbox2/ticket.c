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
