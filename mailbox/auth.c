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

#include <errno.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>

#include <auth0.h>


static int
_authenticate_null (authority_t auth)
{
  (void) auth;
  return 0;
}

int
authority_create_null (authority_t *pauthority, void *owner)
{
  int rc = authority_create(pauthority, NULL, owner);
  if(rc)
    return rc;
  (*pauthority)->_authenticate = _authenticate_null;
  return 0;
}

int
authority_create (authority_t *pauthority, ticket_t ticket, void *owner)
{
  authority_t authority;
  if (pauthority == NULL)
    return EINVAL;
  authority = calloc (1, sizeof (*authority));
  if (authority == NULL)
    return ENOMEM;
  authority->ticket = ticket;
  authority->owner = owner;
  *pauthority = authority;
  return 0;
}

void
authority_destroy (authority_t *pauthority, void *owner)
{
  if (pauthority && *pauthority)
    {
      authority_t authority = *pauthority;
      if (authority->owner == owner)
	{
	  ticket_destroy (&(authority->ticket), authority);
	  free (authority);
	}
      *pauthority = NULL;
    }
}

void *
authority_get_owner (authority_t authority)
{
  return (authority) ? authority->owner : NULL;
}

int
authority_set_ticket (authority_t authority, ticket_t ticket)
{
  if (authority == NULL)
    return EINVAL;
  if (authority->ticket)
    ticket_destroy (&(authority->ticket), authority);
  authority->ticket = ticket;
  return 0;
}

int
authority_get_ticket (authority_t authority, ticket_t *pticket)
{
  if (authority == NULL || pticket == NULL)
    return EINVAL;
  if (authority->ticket == NULL)
    {
      int status = ticket_create (&(authority->ticket), authority);
      if (status != 0)
	return status;
    }
  *pticket = authority->ticket;
  return 0;
}

int
authority_authenticate (authority_t authority)
{
  if (authority && authority->_authenticate)
    {
      return authority->_authenticate (authority);
    }
  return EINVAL;
}

int
authority_set_authenticate (authority_t authority,
			    int (*_authenticate) __P ((authority_t)),
			    void *owner)
{
  if (authority == NULL)
    return EINVAL;

  if (authority->owner != owner)
    return EACCES;
  authority->_authenticate = _authenticate;
  return 0;
}
