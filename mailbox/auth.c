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

#include <auth.h>
#include <cpystr.h>

#include <errno.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>

struct _auth
{
  char *login;
  char *passwd;
  uid_t owner;
  gid_t group;
  mode_t mode;
};

int
auth_init (auth_t *pauth)
{
  auth_t auth;
  if (pauth == NULL)
    return EINVAL;
  auth = calloc (1, sizeof (*auth));
  if (auth == NULL)
    return ENOMEM;
  auth->owner = auth->group = -1;
  auth->mode = 0600;
  *pauth = auth;
  return 0;
}

void
auth_destroy (auth_t *pauth)
{
  if (pauth && *pauth)
    {
      auth_t auth = *pauth;
      free (auth->login);
      free (auth->passwd);
      free (auth);
      *pauth = NULL;
    }
}

/* login/passwd */
int
auth_get_login  (auth_t auth, char *login, size_t len, size_t *n)
{
  size_t nwrite = 0;
  if (auth == NULL)
    return EINVAL;
  nwrite = _cpystr (login, auth->login, len);
  if (n)
    *n = nwrite;
  return 0;
}

int
auth_set_login  (auth_t auth, const char *login, size_t len)
{
  char *log = NULL;
  if (auth == NULL)
    return EINVAL;
  if (login != NULL)
    {
      log = calloc (len + 1, sizeof (char));
      if (log == NULL)
	return ENOMEM;
      memcpy (log, login, len);
    }
  free (auth->login);
  auth->login = log;
  return 0;
}

int
auth_get_passwd  (auth_t auth, char *passwd, size_t len, size_t *n)
{
  size_t nwrite = 0;
  if (auth == NULL)
    return EINVAL;
  nwrite = _cpystr (passwd, auth->passwd, len);
  if (n)
    *n = nwrite;
  return 0;
}

int
auth_set_passwd  (auth_t auth, const char *passwd, size_t len)
{
  char *pass = NULL;
  if (auth == NULL)
    return EINVAL;
  if (passwd != NULL)
    {
      char *pass = calloc (len + 1, sizeof (char));
      if (pass == NULL)
	return ENOMEM;
      memcpy (pass, passwd, len);
    }
  free (auth->passwd);
  auth->passwd = pass;
  return 0;
}

/* owner and group */
int
auth_get_owner (auth_t auth, uid_t *powner)
{
  if (auth == NULL)
    return EINVAL;
  if (powner)
    *powner = auth->owner;
  return 0;
}

int
auth_set_owner (auth_t auth, uid_t owner)
{
  if (auth == NULL)
    return 0;
  auth->owner = owner;
  return 0;
}

int
auth_get_group (auth_t auth, gid_t *pgroup)
{
  if (auth == NULL)
    return EINVAL;
  if (pgroup)
    *pgroup = auth->group;
  return 0;
}

int
auth_set_group (auth_t auth, gid_t group)
{
  if (auth == NULL)
    return EINVAL;
  auth->group = group;
  return 0;
}

int
auth_get_mode (auth_t auth, mode_t *pmode)
{
  if (auth == NULL)
    return EINVAL;
  if (pmode)
    *pmode = auth->mode;
  return 0;
}

int
auth_set_mode (auth_t auth, mode_t mode)
{
  if (auth == NULL)
    return EINVAL;
  auth->mode = mode;
  return 0;
}
