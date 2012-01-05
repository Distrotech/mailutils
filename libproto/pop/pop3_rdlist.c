/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003-2005, 2007, 2010-2012 Free Software Foundation,
   Inc.

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

#include <string.h>
#include <mailutils/stream.h>
#include <mailutils/list.h>
#include <mailutils/cctype.h>
#include <mailutils/cstr.h>
#include <mailutils/sys/pop3.h>

int
mu_pop3_stream_to_list (mu_pop3_t pop3, mu_stream_t stream, mu_list_t list)
{
  int status;
  size_t n;
  
  while ((status = mu_stream_getline (stream, &pop3->rdbuf, &pop3->rdsize, &n))
	 == 0
	 && n > 0)
    {
      char *np = strdup (pop3->rdbuf);
      if (!np)
	{
	  status = ENOMEM;
	  break;
	}
      mu_rtrim_class (np, MU_CTYPE_SPACE);
      status = mu_list_append (list, np);
      if (status)
	break;
    }
  return status;
}

int
mu_pop3_read_list (mu_pop3_t pop3, mu_list_t list)
{
  mu_stream_t stream;
  int status = mu_pop3_stream_create (pop3, &stream);
  if (status)
    return status;
  status = mu_pop3_stream_to_list (pop3, stream, list);
  mu_stream_destroy (&stream);
  return status;
}

