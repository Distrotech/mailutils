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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif

#include <mailutils/types.h>
#include <mailutils/cstr.h>
#include <mailutils/sys/url.h>

#define is_wildcard(s) ((s)[0] == '*' && s[1] == 0)

#define WEIGHT_SCHEME 3
#define WEIGHT_USER   4
#define WEIGHT_HOST   2
#define WEIGHT_PORT   1

int
mu_url_matches_ticket (mu_url_t ticket, mu_url_t url, int *pwc)
{
  int wcnt = 0;
					      
  if (is_wildcard (ticket->scheme))
    wcnt += WEIGHT_SCHEME;
  else if (mu_c_strcasecmp (ticket->scheme, url->scheme))
    return 0;

  if (ticket->flags & MU_URL_HOST)
    {
      if (is_wildcard (ticket->host))
	wcnt += WEIGHT_HOST;
      else if (url->flags & MU_URL_HOST)
	{
	  if (mu_c_strcasecmp (ticket->host, url->host))
	    /* FIXME: Compare IP addresses */
	    return 0;
	}
      else
	return 0;
    }
  else
    wcnt += WEIGHT_HOST;

  if (ticket->flags & MU_URL_PORT)
    {
      if (is_wildcard (ticket->portstr))
	wcnt += WEIGHT_PORT;
      else if (url->port & MU_URL_PORT)
	{
	  if (ticket->port != url->port)
	    return 0;
	  else
	    wcnt += WEIGHT_PORT;
	}
    }
  else
    wcnt += WEIGHT_PORT;
  
  if (ticket->flags & MU_URL_USER)
    {
      if (is_wildcard (ticket->user))
	wcnt += WEIGHT_USER;
      
      /* If ticket has a user or pass, but url doesn't, that's OK, we were
	 looking for this info. But if url does have a user/pass, it
	 must match the ticket. */
      else if (url->flags & MU_URL_USER)
	{
	  if (strcmp (ticket->user, url->user))
	    return 0;
	}
    }
  else
    wcnt += WEIGHT_USER;
  
  /* Guess it matches. */
  if (pwc)
    *pwc = wcnt;
  return 1;
}
