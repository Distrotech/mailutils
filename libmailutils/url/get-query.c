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
#include <mailutils/errno.h>
#include <mailutils/argcv.h>
#include <mailutils/sys/url.h>

int
mu_url_sget_query (const mu_url_t url, size_t *qc, char ***qv)
{
  if (url == NULL)
    return EINVAL;
  /* See FIXME below */
  *qc = url->qargc;
  *qv = url->qargv;
  return 0;
}

int
mu_url_aget_query (const mu_url_t url, size_t *qc, char ***qv)
{
  size_t qargc, i;
  char **qargv;
  char **qcopy;

  int rc = mu_url_sget_fvpairs (url, &qargc, &qargv);
  if (rc)
    return rc;

  qcopy = calloc (qargc + 1, sizeof (qcopy[0]));
  if (!qcopy)
    return errno;
  for (i = 0; i < qargc; i++)
    {
      if (!(qcopy[i] = strdup (qargv[i])))
	{
	  mu_argcv_free (i, qcopy);
	  return errno;
	}
    }
  qcopy[i] = NULL;
  *qc = qargc;
  *qv = qcopy;
  return 0;
}

