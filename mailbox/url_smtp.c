/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2005 Free Software Foundation, Inc.

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

#ifdef ENABLE_SMTP

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <registrar0.h>
#include <url0.h>

int
_url_smtp_init (url_t url)
{
  int status = 0;

  if((status = url_parse(url)))
    return status;

  if(!url_is_scheme(url, "smtp"))
    return EINVAL;

  /* host isn't optional */
  if(!url->host)
    return EINVAL;

  /* port has a default */
  if(!url->port)
    url->port = MU_SMTP_PORT;

  /* accept url->user, pass, and auth
     for the ESMTP authentication */

  /* all other fields must be NULL */
  if(url->path || url->query)
    return EINVAL;

  return 0;
}

#endif /* ENABLE_SMTP */
