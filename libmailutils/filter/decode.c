/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2003, 2005-2007, 2009-2012 Free Software
   Foundation, Inc.

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

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <mailutils/stream.h>
#include <mailutils/filter.h>
#include <mailutils/cstr.h>

enum mu_iconv_fallback_mode mu_default_fallback_mode = mu_fallback_copy_octal;

int
mu_set_default_fallback (const char *str)
{
  if (strcmp (str, "none") == 0)
    mu_default_fallback_mode = mu_fallback_none;
  else if (strcmp (str, "copy-pass") == 0)
    mu_default_fallback_mode = mu_fallback_copy_pass;
  else if (strcmp (str, "copy-octal") == 0)
    mu_default_fallback_mode = mu_fallback_copy_octal;
  else
    return EINVAL;
  return 0;
}

int
mu_decode_filter_args (mu_stream_t *pfilter, mu_stream_t input,
		       const char *filter_name, int argc, const char **argv,
		       const char *fromcode, const char *tocode)
{
  int xargc, i;
  char **xargv;
  int rc;
  
  xargc = argc + 5;
  xargv = calloc (xargc + 1, sizeof (xargv[0]));
  if (!xargv)
    return ENOMEM;

  i = 0;
  if (filter_name)
    xargv[i++] = (char*) filter_name;
  for (; i < argc; i++)
    xargv[i] = (char*) argv[i];

  if (i)
    xargv[i++] = "+";
  xargv[i++] = "ICONV";
  xargv[i++] = (char*) fromcode;
  xargv[i++] = (char*) tocode;
  xargv[i] = NULL;

  rc = mu_filter_chain_create (pfilter, input,
			       MU_FILTER_DECODE, MU_STREAM_READ,
			       i, xargv);
  free (xargv);
  return rc;
}


int
mu_decode_filter (mu_stream_t *pfilter, mu_stream_t input,
		  const char *filter_name,
		  const char *fromcode, const char *tocode)
{
  return mu_decode_filter_args (pfilter, input, filter_name, 0, NULL,
				fromcode, tocode);
}

