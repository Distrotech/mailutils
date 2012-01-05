/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2010-2012 Free Software Foundation, Inc.

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

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>

#ifndef MAXHOSTNAMELEN
# define MAXHOSTNAMELEN 64
#endif

int
mu_get_host_name (char **host)
{
  char *hostname = NULL;
  size_t size = 0;
  char *p;

  while (1)
    {
      if (size == 0)
	{
	  size = MAXHOSTNAMELEN;
	  p = malloc (size);
	}
      else
	{
	  size_t ns = size * 2;
	  if (ns < size)
	    {
	      free (hostname);
	      return ENOMEM;
	    }
	  size = ns;
	  p = realloc (hostname, size);
	}
      if (!p)
	{
	  free (hostname);
	  return ENOMEM;
	}
      hostname = p;
      hostname[size - 1] = 0;
      if (gethostname (hostname, size - 1) == 0)
	{
	  if (!hostname[size - 1])
	    break;
	}
      else if (errno != 0 && errno != ENAMETOOLONG && errno != EINVAL
	       && errno != ENOMEM)
	{
	  int rc = errno;
	  free (hostname);
	  return rc;
	}
    }

  /* Try to return fully qualified host name */
  if (!strchr (hostname, '.'))
    {
      struct hostent *hp = gethostbyname (hostname);
      if (hp)
	{
	  size_t len = strlen (hp->h_name);
	  if (size < len + 1)
	    {
	      p = realloc (hostname, len + 1);
	      if (!p)
		{
		  free (hostname);
		  return ENOMEM;
		}
	      hostname = p;
	    }
	  strcpy (hostname, hp->h_name);
	}
    }
  
  *host = hostname;
  return 0;
}  

