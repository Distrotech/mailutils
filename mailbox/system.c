/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 2002 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */


#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#ifdef HAVE_SHADOW_H
# include <shadow.h>
#endif
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif
#ifdef HAVE_CRYPT_H
# include <crypt.h>
#endif

#include <mailutils/list.h>
#include <mailutils/iterator.h>
#include <mailutils/mailbox.h>
#include <mailutils/argp.h>
#include <mailutils/mu_auth.h>

/* System database */
static int
mu_auth_system (void *return_data, struct passwd *pw)
{
  char *mailbox_name;
  int rc;
  
  if (!pw)
    return 1;

  mailbox_name = malloc (strlen (mu_path_maildir) +
			 strlen (pw->pw_name) + 1);
  if (!mailbox_name)
    return 1;

  sprintf (mailbox_name, "%s%s", mu_path_maildir, pw->pw_name);
  
  rc = mu_auth_data_alloc ((struct mu_auth_data **) return_data,
			   pw->pw_name,
			   pw->pw_passwd,
			   pw->pw_uid,
			   pw->pw_gid,
			   pw->pw_gecos,
			   pw->pw_dir,
			   pw->pw_shell,
			   mailbox_name,
			   1);
  free (mailbox_name);
  return rc;
}

int
mu_auth_system_by_name (void *return_data, void *key,
			void *unused_func_data, void *unused_call_data)
{
  if (!key)
    {
      errno = EINVAL;
      return 1;
    }
  return mu_auth_system (return_data, getpwnam (key));
}

static int
mu_auth_system_by_uid (void *return_data, void *key,
		       void *unused_func_data, void *unused_call_data)
{
  if (!key)
    {
      errno = EINVAL;
      return 1;
    }
  return mu_auth_system (return_data, getpwuid (*(uid_t*) key));
}

static int
mu_authenticate_generic (void *ignored_return_data,
			 void *key,
			 void *ignored_func_data,
			 void *call_data)
{
  struct mu_auth_data *auth_data = key;
  char *pass = call_data;

  return !auth_data
    || !auth_data->passwd
    || strcmp (auth_data->passwd, crypt (pass, auth_data->passwd));
}

/* Called only if generic fails */
static int
mu_authenticate_system (void *ignored_return_data,
			void *key,
			void *ignored_func_data,
			void *call_data)
{
  struct mu_auth_data *auth_data = key;
  char *pass = call_data;

#ifdef HAVE_SHADOW_H
  if (auth_data)
    {
      struct spwd *spw;
      spw = getspnam (auth_data->name);
      if (spw)
	return strcmp (spw->sp_pwdp, crypt (pass, spw->sp_pwdp));
    }
#endif
  return 1;
}


struct mu_auth_module mu_auth_system_module = {
  "system",
  NULL,
  mu_authenticate_system,
  NULL,
  mu_auth_system_by_name,
  NULL,
  mu_auth_system_by_uid,
  NULL
};


struct mu_auth_module mu_auth_generic_module = {
  "generic",
  NULL,
  mu_authenticate_generic,
  NULL,
  mu_auth_nosupport,
  NULL,
  mu_auth_nosupport,
  NULL
};

