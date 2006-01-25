/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2003, 2004, 
   2005 Free Software Foundation, Inc.

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

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <pwd.h>
#include <unistd.h>

#ifdef HAVE_PATHS_H
# include <paths.h>
#endif

#include <mailutils/mailbox.h>
#include <mailutils/mutil.h>
#include <mailutils/error.h>
#include <mailutils/errno.h>
#include <mailutils/mu_auth.h>

static char *_default_mail_dir = MU_PATH_MAILDIR;
static char *_mu_mail_dir;
static char *_default_folder_dir = "Mail";
static char *_mu_folder_dir;

int
mu_set_mail_directory (const char *p)
{
  if (_mu_mail_dir != _default_mail_dir)
    free (_mu_mail_dir);
  return mu_normalize_mailbox_url (&_mu_mail_dir, p);
}

void
mu_set_folder_directory (const char *p)
{
  if (_mu_folder_dir != _default_folder_dir)
    free (_mu_folder_dir);
  _mu_folder_dir = strdup (p);
}

const char *
mu_mail_directory ()
{
  if (!_mu_mail_dir)
    _mu_mail_dir = _default_mail_dir;
  return _mu_mail_dir;
}

const char *
mu_folder_directory ()
{
  if (!_mu_folder_dir)
    _mu_folder_dir = _default_folder_dir;
  return _mu_folder_dir;
}

int
mu_construct_user_mailbox_url (char **pout, const char *name)
{
  const char *p = mu_mail_directory ();
  *pout = malloc (strlen (p) + strlen (name) + 1);
  if (!*pout)
    return errno;
  strcat (strcpy (*pout, p), name);
  return 0;
}

/* Is this a security risk?  */
#define USE_ENVIRON 1

static int
split_shortcut (const char *file, const char pfx[], char **user, char **rest)
{
  *user = NULL;
  *rest = NULL;

  if (!strchr (pfx, file[0]))
    return 0;

  if (*++file == 0)
    return 0;
  else
    {
      char *p = strchr (file, '/');
      int len;
      if (p)
        len = p - file + 1;
      else
        len = strlen (file) + 1;

      if (len == 1)
	*user = NULL;
      else
	{
	  *user = calloc (1, len);
	  if (!*user)
	    return ENOMEM;

	  memcpy (*user, file, len);
	  (*user)[len-1] = 0;
	}
      file += len-1;
      if (file[0] == '/')
        file++;
    }

  if (file[0])
    {
      *rest = strdup (file);
      if (!*rest)
        {
          free (*user);
          return ENOMEM;
        }
    }
  
  return 0;
}

static char *
get_homedir (const char *user)
{
  char *homedir = NULL;
  struct mu_auth_data *auth = NULL;
  
  if (user)
    {
      auth = mu_get_auth_by_name (user);
      if (auth)
        homedir = auth->dir;
    }
  else
    {
#ifdef USE_ENVIRON
      /* NOTE: Should we honor ${HOME}?  */
      homedir = getenv ("HOME");
      if (homedir == NULL)
        {
	  auth = mu_get_auth_by_name (user);
	  if (auth)
	    homedir = auth->dir;
        }
#else
      auth = mu_get_auth_by_name (user);
      if (auth)
	homedir = auth->dir;
#endif
    }

  if (homedir)
    homedir = strdup (homedir);
  mu_auth_data_free (auth);
  return homedir;
}

static int
user_mailbox_name (const char *user, char **mailbox_name)
{
  char *p;
  const char *url = mu_mail_directory ();

  p = strchr (url, ':');
  if (p && strncmp (url, "mbox", p - url))
    {
      *mailbox_name = strdup (url);
      return 0;
    }
  
#ifdef USE_ENVIRON
  if (!user)
    user = (getenv ("LOGNAME")) ? getenv ("LOGNAME") : getenv ("USER");
#endif

  if (user)
    {
      int rc = mu_construct_user_mailbox_url (mailbox_name, user);
      if (rc)
	return rc;
    }
  else
    {
      struct mu_auth_data *auth = mu_get_auth_by_uid (getuid ());

      if (!auth)
        {
          mu_error ("Who am I ?\n");
          return EINVAL;
        }
      *mailbox_name = strdup (auth->mailbox);
      mu_auth_data_free (auth);
    }

  return 0;
}

static int
plus_expand (const char *file, char **buf)
{
  char *user = NULL;
  char *path = NULL;
  char *home;
  const char *folder_dir = mu_folder_directory ();
  int status, len;
  
  if ((status = split_shortcut (file, "+=", &path, &user)))
    return status;

  if (!path)
    {
      free (user);
      return ENOENT;
    }
  
  home = get_homedir (user);
  if (!home)
    {
      free (user);
      free (path);
      return ENOENT;
    }

  if (folder_dir[0] == '/' || mu_is_proto (folder_dir))
    {
      len = strlen (folder_dir) + strlen (path) + 2;
      *buf = malloc (len);
      sprintf (*buf, "%s/%s", folder_dir, path);
    }
  else
    {
      len = strlen (home) + strlen (folder_dir) + strlen (path) + 3;
      *buf = malloc (len);
      sprintf (*buf, "%s/%s/%s", home, folder_dir, path);
    }
  (*buf)[len-1] = 0;
  
  free (user);
  free (path);
  free (home);
  return 0;
}

static int
percent_expand (const char *file, char **mbox)
{
  char *user = NULL;
  char *path = NULL;
  int status;
  
  if ((status = split_shortcut (file, "%", &user, &path)))
    return status;

  if (path)
    {
      free (user);
      free (path);
      return ENOENT;
    }

  status = user_mailbox_name (user, mbox);
  free (user);
  return status;
}

/* We are trying to be smart about the location of the mail.
   mu_mailbox_create() is not doing this.
   %           --> system mailbox for the real uid
   %user       --> system mailbox for the given user
   ~/file      --> /home/user/file
   ~user/file  --> /home/user/file
   +file       --> /home/user/Mail/file
   =file       --> /home/user/Mail/file
*/
int
mu_mailbox_create_default (mu_mailbox_t *pmbox, const char *mail)
{
  char *mbox = NULL;
  char *tmp_mbox = NULL;
  char *p;
  int status = 0;

  /* Sanity.  */
  if (pmbox == NULL)
    return MU_ERR_OUT_PTR_NULL;

  /* Other utilities may not understand GNU mailutils url namespace, so
     use FOLDER instead, to not confuse others by using MAIL.  */
  if (mail == NULL || *mail == '\0')
    {
      mail = getenv ("FOLDER");

      /* Fallback to wellknown environment.  */
      if (!mail)
        mail = getenv ("MAIL");

      if (!mail)
        {
          if ((status = user_mailbox_name (NULL, &tmp_mbox)))
            return status;
          mail = tmp_mbox;
        }
    }

  p = mu_tilde_expansion (mail, "/", NULL);
  if (tmp_mbox)
    {
      free (tmp_mbox);
      tmp_mbox = p;
    }
  mail = p;
  if (!mail)
    return ENOMEM;
  
  switch (mail[0])
    {
    case '%':
      status = percent_expand (mail, &mbox);
      break;
      
    case '+':
    case '=':
      status = plus_expand (mail, &mbox);
      break;

    case '/':
      mbox = strdup (mail);
      break;
      
    default:
      if (!mu_is_proto (mail))
	{
	  tmp_mbox = mu_getcwd();
	  mbox = malloc (strlen (tmp_mbox) + strlen (mail) + 2);
	  sprintf (mbox, "%s/%s", tmp_mbox, mail);
	}
      else
	mbox = strdup (mail);
      break;
    }

  if (tmp_mbox)
    free (tmp_mbox);

  if (status)
    return status;
  
  status = mu_mailbox_create (pmbox, mbox);
  free (mbox);
  return status;
}
