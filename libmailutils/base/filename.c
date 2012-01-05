/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2003, 2005-2007, 2009-2012 Free Software
   Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library.  If not, see 
   <http://www.gnu.org/licenses/>. */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <mailutils/types.h>
#include <mailutils/util.h>
#include <mailutils/mu_auth.h>

char *
mu_get_homedir (void)
{
  char *homedir = getenv ("HOME");
  if (homedir)
    homedir = strdup (homedir);
  else
    {
      struct mu_auth_data *auth = mu_get_auth_by_uid (geteuid ());
      if (!auth)
	return NULL;
      homedir = strdup (auth->dir);
      mu_auth_data_free (auth);
    }
  return homedir;
}

char *
mu_get_full_path (const char *file)
{
  char *p = NULL;

  if (!file)
    p = mu_getcwd ();
  else if (*file != '/')
    {
      char *cwd = mu_getcwd ();
      if (cwd)
	{
	  p = mu_make_file_name (cwd, file);
	  free (cwd);
	}
    }
  else
    p = strdup (file);
  return p;
}

/* mu_normalize_path: convert pathname containig relative paths specs (../)
   into an equivalent absolute path. Strip trailing delimiter if present,
   unless it is the only character left. E.g.:

         /home/user/../smith   -->   /home/smith
	 /home/user/../..      -->   /

*/
char *
mu_normalize_path (char *path)
{
  int len;
  char *p;

  if (!path)
    return path;

  len = strlen (path);

  /* Empty string is returned as is */
  if (len == 0)
    return path;

  /* delete trailing delimiter if any */
  if (len && path[len-1] == '/')
    path[len-1] = 0;

  /* Eliminate any /../ */
  for (p = strchr (path, '.'); p; p = strchr (p, '.'))
    {
      if (p > path && p[-1] == '/')
	{
	  if (p[1] == '.' && (p[2] == 0 || p[2] == '/'))
	    /* found */
	    {
	      char *q, *s;

	      /* Find previous delimiter */
	      for (q = p-2; *q != '/' && q >= path; q--)
		;

	      if (q < path)
		break;
	      /* Copy stuff */
	      s = p + 2;
	      p = q;
	      while ((*q++ = *s++))
		;
	      continue;
	    }
	}

      p++;
    }

  if (path[0] == 0)
    {
      path[0] = '/';
      path[1] = 0;
    }

  return path;
}

/* Expand a PATTERN to the pathname. PATTERN may contain the following
   macro-notations:
   ---------+------------ 
   notation |  expands to
   ---------+------------
   %u         user name
   %h         user's home dir
   ~          Likewise
   ---------+------------

   Allocates memory. 
*/
char *
mu_expand_path_pattern (const char *pattern, const char *username)
{
  const char *p;
  char *q;
  char *path;
  size_t len = 0;
  struct mu_auth_data *auth = NULL;
  
  for (p = pattern; *p; p++)
    {
      if (*p == '~')
        {
          if (!auth)
            {
              auth = mu_get_auth_by_name (username);
              if (!auth)
                return NULL;
            }
          len += strlen (auth->dir);
        }
      else if (*p == '%')
	switch (*++p)
	  {
	  case 'u':
	    len += strlen (username);
	    break;
	    
	  case 'h':
	    if (!auth)
	      {
		auth = mu_get_auth_by_name (username);
		if (!auth)
		  return NULL;
	      }
	    len += strlen (auth->dir);
	    break;
	    
	  case '%':
	    len++;
	    break;
	    
	  default:
	    len += 2;
	  }
      else
	len++;
    }
  
  path = malloc (len + 1);
  if (!path)
    return NULL;

  p = pattern;
  q = path;
  while (*p)
    {
      size_t off = strcspn (p, "~%");
      memcpy (q, p, off);
      q += off;
      p += off;
      if (*p == '~')
	{
	  strcpy (q, auth->dir);
	  q += strlen (auth->dir);
	  p++;
	}
      else if (*p == '%')
	{
	  switch (*++p)
	    {
	    case 'u':
	      strcpy (q, username);
	      q += strlen (username);
	      break;
	    
	    case 'h':
	      strcpy (q, auth->dir);
	      q += strlen (auth->dir);
	      break;
	  
	    case '%':
	      *q++ = '%';
	      break;
	  
	    default:
	      *q++ = '%';
	      *q++ = *p;
	    }
	  p++;
	}
    }

  *q = 0;
  if (auth)
    mu_auth_data_free (auth);
  return path;
}
