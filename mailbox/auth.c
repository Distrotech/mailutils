/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2004 Free Software Foundation, Inc.

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

#include <mailutils/errno.h>
#include <auth0.h>

static int
_authenticate_null (authority_t auth ARG_UNUSED)
{
  return 0;
}

int
authority_create_null (authority_t *pauthority, void *owner)
{
  int rc = authority_create(pauthority, NULL, owner);
  if (rc)
    return rc;
  authority_set_authenticate (*pauthority, _authenticate_null,  owner);
  return 0;
}

int
authority_create (authority_t *pauthority, ticket_t ticket, void *owner)
{
  authority_t authority;
  if (pauthority == NULL)
    return MU_ERR_OUT_PTR_NULL;
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
  if (authority == NULL)
    return EINVAL;
  if (pticket == NULL)
    return MU_ERR_OUT_PTR_NULL;
  if (authority->ticket == NULL)
    {
      int status = ticket_create (&(authority->ticket), authority);
      if (status != 0)
	return status;
    }
  *pticket = authority->ticket;
  return 0;
}

struct auth_cb
{
  int status;
  authority_t authority;
};

static int
try_auth (void *item, void *data)
{
  int (*authenticate) __P ((authority_t)) = item;
  struct auth_cb *cb = data;
  if (authenticate (cb->authority) == 0)
    {
      cb->status = 0;
      return 1;
    }
  return 0;
}

int
authority_authenticate (authority_t authority)
{
  if (authority && authority->auth_methods)
    {
      struct auth_cb cb;
      cb.status = MU_ERR_AUTH_FAILURE;
      cb.authority = authority;
      list_do (authority->auth_methods, try_auth, &cb);
      return cb.status;
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
  if (!authority->auth_methods)
    {
      int rc = list_create (&authority->auth_methods);
      if (rc)
	return rc;
    }
  list_append (authority->auth_methods, _authenticate);
  return 0;
}
