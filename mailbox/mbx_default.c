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

/* Is this a security risk?  */
#define USE_ENVIRON 1

static char * tilde_expansion      __P ((const char *));
static char * plus_equal_expansion __P ((const char *));
static char * get_cwd              __P ((void));
static char * get_full_path        __P ((const char *));
static const char * get_homedir    __P ((const char *));

/* Do + and = expansion to ~/Mail, if necessary. */
static char *
plus_equal_expansion (const char *file)
{
  char *p = NULL;
  if (file && (*file == '+' || *file == '='))
    {
      char *folder;
      /* Skip '+' or '='.  */
      file++;
      folder = tilde_expansion ("~/Mail");
      if (folder)
	{
	  p = malloc (strlen (folder) + 1 + strlen (file) + 1);
	  if (p)
	    sprintf(p, "%s/%s", folder, file);
	  free (folder);
	}
    }

  if (!p)
    p = strdup (file);
  return p;
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
	  pw = getpwuid (getuid ());
	  if (pw)
	    homedir = pw->pw_dir;
	}
#else
      pw = getpwuid (getuid ());
      if (pw)
	homedir = pw->pw_dir;
#endif
    }
  return homedir;
}

/* Do ~ , if necessary.  We do not use $HOME. */
static char *
tilde_expansion (const char *file)
{
  char *p = NULL;
  const char *homedir = NULL;
  const char delim = '/'; /* Not portable.  */

  if (file && *file == '~')
    {
      /* Skip the tilde.  */
      file++;

      /* This means we have ~ or ~/something.  */
      if (*file == delim || *file == '\0')
        {
	  homedir = get_homedir (NULL);
	  if (homedir)
	    {
	      p = calloc (strlen (homedir) + strlen (file) + 1, 1);
	      if (p)
		{
		  strcpy (p, homedir);
		  strcat (p, file);
		}
	    }
	}
      /* Means we have ~user or ~user/something.  */
      else
        {
          const char *s = file;

	  /* Move  to the first delim.  */
          while (*s && *s != delim) s++;

	  /* Get the username homedir.  */
	  {
	    char *name;
	    name = calloc (s - file + 1, 1);
	    if (name)
	      {
		memcpy (name, file, s - file);
		name [s - file] = '\0';
	      }
	    homedir = get_homedir (name);
	    free (name);
	  }

          if (homedir)
            {
              p = calloc (strlen (homedir) + strlen (s) + 1, 1);
	      if (p)
		{
		  strcpy (p, homedir);
		  strcat (p, s);
		}
            }
        }
    }

  if (!p)
    p = strdup (file);
  return p;
}

static char *
get_cwd ()
{
  char *ret;
  unsigned path_max;
  char buf[128];

  errno = 0;
  ret = getcwd (buf, sizeof (buf));
  if (ret != NULL)
    return strdup (buf);

  if (errno != ERANGE)
    return NULL;

  path_max = 128;
  path_max += 2;                /* The getcwd docs say to do this. */

  for (;;)
    {
      char *cwd = (char *) malloc (path_max);

      errno = 0;
      ret = getcwd (cwd, path_max);
      if (ret != NULL)
        return ret;
      if (errno != ERANGE)
        {
          int save_errno = errno;
          free (cwd);
          errno = save_errno;
          return NULL;
        }

      free (cwd);

      path_max += path_max / 16;
      path_max += 32;
    }
  /* oops?  */
  return NULL;
}

static char *
get_full_path (const char *file)
{
  char *p = NULL;

  if (!file)
    p = get_cwd ();
  else if (*file != '/')
    {
      char *cwd = get_cwd ();
      if (cwd)
	{
	  p = calloc (strlen (cwd) + 1 + strlen (file) + 1, 1);
	  if (p)
	    sprintf (p, "%s/%s", cwd, file);
	  free (cwd);
	}
    }

  if (!p)
    p = strdup (file);
  return p;
}

/* We are trying to be smart about the location of the mail.
   mailbox_create() is not doing this.
   ~/file  --> /home/user/file
   ~user/file --> /home/user/file
   +file --> /home/user/Mail/file
   =file --> /home/user/Mail/file
*/
int
mailbox_create_default (mailbox_t *pmbox, const char *mail)
{
  char *mbox = NULL;
  int status;

  /* Sanity.  */
  if (pmbox == NULL)
    return EINVAL;

  /* Other utilities may not understand GNU mailutils url namespace, so
     use FOLDER instead, to not confuse others by using MAIL.  */
  if (mail == NULL || *mail == '\0')
    mail = getenv ("FOLDER");

  /* Fallback to wellknown environment.  */
  if (mail == NULL)
    mail = getenv ("MAIL");

  /* FIXME: This is weak, it would be better to check
     for all the known schemes to detect presence of URLs.  */
  if (mail && *mail && strchr (mail, ':') == NULL)
    {
      char *mail0;
      char *mail2;

      mail0 = tilde_expansion (mail);
      mail2 = plus_equal_expansion (mail0);
      free (mail0);
      mbox = get_full_path (mail2);
      free (mail2);
    }
  else if (mail)
    mbox = strdup (mail);

  /* Search the spooldir.  */
  if (mbox == NULL)
    {
      const char *user = NULL;
#ifdef USE_ENVIRON
      user = (getenv ("LOGNAME")) ? getenv ("LOGNAME") : getenv ("USER");
#endif
      if (user == NULL)
	{
	  struct passwd *pw;
	  pw = getpwuid (getuid ());
	  if (pw)
	    user = pw->pw_name;
	  else
	    {
	      mu_error ("Who am I ?\n");
	      return EINVAL;
	    }
	}
      mbox = malloc (strlen (user) + strlen (MU_PATH_MAILDIR) + 2);
      if (mbox == NULL)
	return ENOMEM;
      sprintf (mbox, "%s%s", MU_PATH_MAILDIR, user);
    }
  status = mailbox_create (pmbox, mbox);
  free (mbox);
  return status;
}
