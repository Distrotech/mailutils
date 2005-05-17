/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2002 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library; if not, write to the
   Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301 USA */

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
#ifdef HAVE_SECURITY_PAM_APPL_H
# include <security/pam_appl.h>
#endif
#ifdef HAVE_CRYPT_H
# include <crypt.h>
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h> /*FIXME!*/

#include <mailutils/list.h>
#include <mailutils/iterator.h>
#include <mailutils/mailbox.h>
#include <mailutils/argp.h>
#include <mailutils/mu_auth.h>
#include <mailutils/nls.h>

#ifdef ENABLE_VIRTUAL_DOMAINS

static char *site_virtual_pwddir = SITE_VIRTUAL_PWDDIR;

static struct passwd *
getpwnam_virtual (const char *u)
{
  struct passwd *pw = NULL;
  FILE *pfile;
  size_t i = 0, len = strlen (u), delim = 0;
  char *filename;

  for (i = 0; i < len && delim == 0; i++)
    if (u[i] == '!' || u[i] == ':' || u[i] == '@')
      delim = i;

  if (delim == 0)
    return NULL;

  filename = malloc (strlen (site_virtual_pwddir) +
		     strlen (&u[delim + 1]) + 2 /* slash and null byte */);
  if (filename == NULL)
    return NULL;

  sprintf (filename, "%s/%s", site_virtual_pwddir, &u[delim + 1]);
  pfile = fopen (filename, "r");
  free (filename);

  if (pfile)
    while ((pw = fgetpwent (pfile)) != NULL)
      {
	if (strlen (pw->pw_name) == delim && !strncmp (u, pw->pw_name, delim))
	  break;
      }

  return pw;
}

static struct passwd *
getpwnam_ip_virtual (const char *u)
{
  struct sockaddr_in addr;
  struct passwd *pw = NULL;
  int len = sizeof (addr);
  
  if (getsockname (0, (struct sockaddr *)&addr, &len) == 0)
    {
      char *ip;
      char *user;

      struct hostent *info = gethostbyaddr ((char *)&addr.sin_addr,
					    4, AF_INET);

      if (info)
	{
	  user = malloc (strlen (info->h_name) + strlen (u) + 2);
	  if (user)
	    {
	      sprintf (user, "%s!%s", u, info->h_name);
	      pw = getpwnam_virtual (user);
	      free (user);
	    }
        }

      if (!pw)
	{
	  ip = inet_ntoa (addr.sin_addr);
	  user = malloc (strlen (ip) + strlen (u) + 2);
	  if (user)
	    {
	      sprintf (user, "%s!%s", u, ip);
	      pw = getpwnam_virtual (user);
	      free (user);
	    }
	}
    }
  return pw;
}

/* Virtual domains */
static int
mu_auth_virt_domain_by_name (struct mu_auth_data **return_data,
			     const void *key,
			     void *unused_func_data, void *unused_call_data)
{
  int rc;
  struct passwd *pw;
  char *mailbox_name;
  
  if (!key)
    {
      errno = EINVAL;
      return 1;
    }

  pw = getpwnam_virtual (key);
  if (!pw)
    {
      pw = getpwnam_ip_virtual (key);
      if (!pw)
	return 1;
    }
  
  mailbox_name = calloc (strlen (pw->pw_dir) + strlen ("/INBOX") + 1, 1);
  sprintf (mailbox_name, "%s/INBOX", pw->pw_dir);

  rc = mu_auth_data_alloc (return_data,
			   pw->pw_name,
			   pw->pw_passwd,
			   pw->pw_uid,
			   pw->pw_gid,
			   pw->pw_gecos,
			   pw->pw_dir,
			   pw->pw_shell,
			   mailbox_name,
			   0);
  free (mailbox_name);
  return rc;
}

#define ARG_PWDDIR 1

static error_t
mu_virt_argp_parser (int key, char *arg, struct argp_state *state)
{
  switch (key)
    {
    case ARG_PWDDIR:
      site_virtual_pwddir = arg;
      break;

    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

static struct argp_option mu_virt_argp_option[] = {
  { "virtual-passwd-dir", ARG_PWDDIR, N_("DIR"), 0,
    N_("Search for virtual passwd file in DIR"), 0},
    { NULL,      0, NULL, 0, NULL, 0 }
};

struct argp mu_virt_argp = {
  mu_virt_argp_option,
  mu_virt_argp_parser,
};

#else
static int
mu_auth_virt_domain_by_name (struct mu_auth_data **return_data ARG_UNUSED,
			     const void *key ARG_UNUSED,
			     void *func_data ARG_UNUSED,
			     void *call_data ARG_UNUSED)
{
  errno = ENOSYS;
  return 1;
}
#endif

struct mu_auth_module mu_auth_virtual_module = {
  "virtdomain",
#ifdef ENABLE_VIRTUAL_DOMAINS
  &mu_virt_argp,
#else
  NULL,
#endif
  mu_auth_nosupport,
  NULL,
  mu_auth_virt_domain_by_name,
  NULL,
  mu_auth_nosupport,
  NULL
};
    
