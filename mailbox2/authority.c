/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Library Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with GNU Mailutils; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <mailutils/error.h>
#include <mailutils/sys/authority.h>

int
authority_ref (authority_t authority)
{
  if (authority == NULL || authority->vtable == NULL
      || authority->vtable->ref == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return authority->vtable->ref (authority);
}

void
authority_destroy (authority_t *pauthority)
{
  if (pauthority && *pauthority)
    {
      authority_t authority = *pauthority;
      if (authority->vtable && authority->vtable->destroy)
	authority->vtable->destroy (pauthority);
      *pauthority = NULL;
    }
}

int
authority_set_ticket (authority_t authority, ticket_t ticket)
{
  if (authority == NULL || authority->vtable == NULL
      || authority->vtable->set_ticket == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return authority->vtable->set_ticket (authority, ticket);
}

int
authority_get_ticket (authority_t authority, ticket_t *pticket)
{
  if (authority == NULL || authority->vtable == NULL
      || authority->vtable->get_ticket == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return authority->vtable->get_ticket (authority, pticket);
}

int
authority_authenticate (authority_t authority)
{
  if (authority == NULL || authority->vtable == NULL
      || authority->vtable->authenticate == NULL)
    return MU_ERROR_NOT_SUPPORTED;
  return authority->vtable->authenticate (authority);
}
