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

char *mu_path_maildir = MU_PATH_MAILDIR; 

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
      
      *user = calloc (1, len);
      if (!*user)
        return ENOMEM;

      memcpy (*user, file, len);
      (*user)[len-1] = 0;
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

static const char *
get_homedir (const char *user)
{
  const char *homedir = NULL;
  struct passwd *pw = NULL;
  if (user)
    {
      pw = mu_getpwnam (user);
      if (pw)
        homedir = pw->pw_dir;
    }
  else
    {
#ifdef USE_ENVIRON
      /* NOTE: Should we honor ${HOME}?  */
      homedir = getenv ("HOME");
      if (homedir == NULL)
        {
          pw = mu_getpwuid (getuid ());
          if (pw)
            homedir = pw->pw_dir;
        }
#else
      pw = mu_getpwuid (getuid ());
      if (pw)
        homedir = pw->pw_dir;
#endif
    }
  return homedir;
}

static int
user_mailbox_name (const char *user, char **mailbox_name)
{
#ifdef USE_ENVIRON
  if (!user)
    user = (getenv ("LOGNAME")) ? getenv ("LOGNAME") : getenv ("USER");
#endif
  if (user == NULL)
    {
      struct passwd *pw;
      pw = mu_getpwuid (getuid ());
      if (pw)
        user = pw->pw_name;
      else
        {
          mu_error ("Who am I ?\n");
          return EINVAL;
        }
    }
  *mailbox_name = malloc (strlen (user) + strlen (mu_path_maildir) + 2);
  if (*mailbox_name == NULL)
    return ENOMEM;
  sprintf (*mailbox_name, "%s%s", mu_path_maildir, user);
  return 0;
}

#define MPREFIX "Mail"

static int
plus_expand (const char *file, char **buf)
{
  char *user = NULL;
  char *path = NULL;
  const char *home;
  int status, len;
  
  if ((status = split_shortcut (file, "+=", &user, &path)))
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

  len = strlen (home) + sizeof (MPREFIX) + strlen (path) + 3;
  *buf = malloc (len);
  sprintf (*buf, "%s/%s/%s", home, MPREFIX, path);
  (*buf)[len-1] = 0;
  free (user);
  free (path);
  return 0;
}

/* Do ~ , if necessary.  We do not use $HOME. */
static int
tilde_expand (const char *file, char **buf)
{
  char *user = NULL;
  char *path = NULL;
  const char *home;
  int status;
  int len;
  
  if ((status = split_shortcut (file, "~", &user, &path)))
    return status;

  if (!user)
    return ENOENT;
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

  free (user); /* not needed anymore */
      
  len = strlen (home) + strlen (path) + 2;
  *buf = malloc (len);
  if (*buf)
    {
      sprintf (*buf, "%s/%s", home, path);
      (*buf)[len-1] = 0;
    }

  free (path);
  free (user);
  return *buf ? 0 : ENOMEM;
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
   mailbox_create() is not doing this.
   %           --> system mailbox for the real uid
   %user       --> system mailbox for the given user
   ~/file      --> /home/user/file
   ~user/file  --> /home/user/file
   +file       --> /home/user/Mail/file
   =file       --> /home/user/Mail/file
*/
int
mailbox_create_default (mailbox_t *pmbox, const char *mail)
{
  char *mbox = NULL;
  char *tmp_mbox = NULL;
  int status = 0;

  /* Sanity.  */
  if (pmbox == NULL)
    return EINVAL;

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

  switch (mail[0])
    {
    case '%':
      status = percent_expand (mail, &mbox);
      break;
    case '~':
      status = tilde_expand (mail, &mbox);
      break;
    case '+':
    case '=':
      status = plus_expand (mail, &mbox);
      break;
    default:
      mbox = strdup (mail);
      break;
    }

  if (tmp_mbox)
    free (tmp_mbox);

  if (status)
    return status;
  
  status = mailbox_create (pmbox, mbox);
  free (mbox);
  return status;
}
