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
#include <mailutils/types.h>
#include <mailutils/cctype.h>
#include <mailutils/util.h>
#include <mailutils/mu_auth.h>

/* NOTE: Allocates Memory.  */
/* Expand: ~ --> /home/user and to ~guest --> /home/guest.  */
char *
mu_tilde_expansion (const char *ref, int delim, const char *homedir)
{
  char *base = strdup (ref);
  char *home = NULL;
  char *proto = NULL;
  size_t proto_len = 0;
  char *p;

  if (!base)
    return NULL;
  for (p = base; *p && mu_isascii (*p) && mu_isalnum (*p); p++)
    ;
  
  if (*p == ':')
    {
      p++;
      proto_len = p - base;
      proto = malloc (proto_len + 1);
      if (!proto)
	return NULL;
      memcpy (proto, base, proto_len);
      proto[proto_len] = 0;
      /* Allow for extra pair of slashes after the protocol specifier */
      if (*p == delim)
	p++;
      if (*p == delim)
	p++;
    }
  else
    p = base;
  
  if (*p == '~')
    {
      p++;
      if (*p == delim || *p == '\0')
        {
	  char *s;
	  if (!homedir)
	    {
	      home = mu_get_homedir ();
	      if (!home)
		return base;
	      homedir = home;
	    }
	  s = calloc (proto_len + strlen (homedir) + strlen (p) + 1, 1);
	  if (proto_len)
	    strcpy (s, proto);
	  else
	    s[0] = 0;
          strcat (s, homedir);
          strcat (s, p);
          free (base);
          base = s;
        }
      else
        {
          struct mu_auth_data *auth;
          char *s = p;
          char *name;
          while (*s && *s != delim)
            s++;
          name = calloc (s - p + 1, 1);
          memcpy (name, p, s - p);
          name[s - p] = '\0';
	  
          auth = mu_get_auth_by_name (name);
          free (name);
          if (auth)
            {
              char *buf = calloc (proto_len + strlen (auth->dir)
				  + strlen (s) + 1, 1);
	      if (proto_len)
		strcpy (buf, proto);
	      else
		buf[0] = 0;
	      strcat (buf, auth->dir);
              strcat (buf, s);
              free (base);
              base = buf;
	      mu_auth_data_free (auth);
            }
        }
    }
  if (home)
    free (home);
  return base;
}

