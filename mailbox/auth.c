/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000 Free Software Foundation, Inc.

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

#include <errno.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>

#include <auth0.h>
#include <cpystr.h>

int
auth_create (auth_t *pauth, void *owner)
{
  auth_t auth;
  if (pauth == NULL)
    return EINVAL;
  auth = calloc (1, sizeof (*auth));
  if (auth == NULL)
    return ENOMEM;
  auth->owner = owner;
  *pauth = auth;
  return 0;
}

void
auth_destroy (auth_t *pauth, void *owner)
{
  if (pauth && *pauth)
    {
      auth_t auth = *pauth;
      if (auth->owner == owner)
	free (auth);
      *pauth = NULL;
    }
}

int
auth_set_authenticate (auth_t auth,
		       int (*_authenticate)(auth_t, char **, char **),
		       void *owner)
{
  if (auth == NULL)
    return EINVAL;
  if (auth->owner != owner)
    return EPERM;
  auth->_authenticate = _authenticate;
  return 0;
}

int
auth_authenticate (auth_t auth, char **user, char **passwd)
{
  if (auth == NULL || auth->_authenticate == NULL)
    return EINVAL;
  return auth->_authenticate (auth, user, passwd);
}

int
auth_set_epilogue (auth_t auth, int (*_epilogue)(auth_t), void *owner)
{
  if (auth == NULL)
    return EINVAL;
  if (auth->owner != owner)
    return EPERM;
  auth->_epilogue = _epilogue;
  return 0;
}

int
auth_epilogue (auth_t auth)
{
  if (auth == NULL || auth->_epilogue == NULL)
    return EINVAL;
  return auth->_epilogue (auth);
}

int
auth_set_prologue (auth_t auth, int (*_prologue)(auth_t), void *owner)
{
  if (auth == NULL)
    return EINVAL;
  if (auth->owner != owner)
    return EPERM;
  auth->_prologue = _prologue;
  return 0;
}

int
auth_prologue (auth_t auth)
{
  if (auth == NULL || auth->_prologue == NULL)
    return EINVAL;
  return auth->_prologue (auth);
}
