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

#include <stdlib.h>
#include <mailutils/types.h>
#include <mailutils/header.h>
#include <mailutils/errno.h>

int
mu_header_sget_firstof (mu_header_t hdr, char **names,
			const char **pval, int *pidx)
{
  int status;
  const char *s = NULL;
  int i;
  
  for (i = 0; names[i]; i++)
    {
      status = mu_header_sget_value (hdr, names[i], &s);
      if (status == 0 && *s != 0)
	{
	  if (pval)
	    *pval = s;
	  if (pidx)
	    *pidx = i;
	  return 0;
	}
    }
  return MU_ERR_NOENT;
}
