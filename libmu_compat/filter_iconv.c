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
#include <mailutils/stream.h>
#include <mailutils/filter.h>

int
mu_filter_iconv_create (mu_stream_t *s, mu_stream_t transport,
			const char *fromcode, const char *tocode, int flags,
			enum mu_iconv_fallback_mode fallback_mode)
{
  const char *argv[] = { "iconv", fromcode, tocode, NULL, NULL };

  switch (fallback_mode)
    {
    case mu_fallback_none:
      argv[3] = "none";
      break;
      
    case mu_fallback_copy_pass:
      argv[3] = "copy-pass";
      break;
      
    case mu_fallback_copy_octal:
      argv[3] = "copy-octal";
    }

  return mu_filter_create_args (s, transport, "iconv", 4, argv,
				MU_FILTER_DECODE,  MU_FILTER_READ);
}

  
