/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2003 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#ifdef ENABLE_IMAP

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif

#include <registrar0.h>
#include <url0.h>

static void url_imap_destroy (url_t url);

static void
url_imap_destroy (url_t url ARG_UNUSED)
{
}

/*
  IMAP URL:
    imap://[<user>[;AUTH=<auth>]@]<host>[/<mailbox>]
  else
    imap://[<user>[:<pass>]@]<host>[/<mailbox>]
*/

int
_url_imap_init (url_t url)
{
  int status = 0;

  url->_destroy = url_imap_destroy;

  status = url_parse (url);

  if (status)
    return status;

  if(!url->host || url->query)
    return EINVAL;

  /* is it pop? */
  if (strcmp ("imap", url->scheme) != 0)
    return EINVAL;

  /* fill in default port, if necesary */
  if (url->port == 0)
    url->port = MU_IMAP_PORT;

  /* fill in default auth, if necessary */
  if (!url->auth)
    {
      url->auth = malloc (1 + 1);
      if (!url->auth)
	return ENOMEM;

      url->auth[0] = '*';
      url->auth[1] = '\0';
    }

  return status;
}

#endif
