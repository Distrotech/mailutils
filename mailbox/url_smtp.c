/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU Library General Public License as published by
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

  /* all other fields must be NULL */
  if(url->user || url->passwd || url->auth || url->path || url->query)
    return EINVAL;

  return 0;
}

