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
#include <mailutils/argcv.h>
#include <mailutils/secret.h>
#include <mailutils/errno.h>
#include <mailutils/sys/url.h>

void
mu_url_destroy (mu_url_t * purl)
{
  if (purl && *purl)
    {
      mu_url_t url = (*purl);

      if (url->_destroy)
	url->_destroy (url);

      if (url->name)
	free (url->name);

      if (url->scheme)
	free (url->scheme);

      if (url->user)
	free (url->user);

      mu_secret_destroy (&url->secret);

      if (url->auth)
	free (url->auth);

      if (url->host)
	free (url->host);

      if (url->path)
	free (url->path);

      if (url->fvcount)
	mu_argcv_free (url->fvcount, url->fvpairs);

      mu_argcv_free (url->qargc, url->qargv);

      free (url);

      *purl = NULL;
    }
}
