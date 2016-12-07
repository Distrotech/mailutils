/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2002, 2005-2008, 2010-2012, 2014-2016 Free
   Software Foundation, Inc.

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
#include <sieve-priv.h>

size_t 
mu_i_sv_string_create (mu_sieve_machine_t mach, char *str)
{
  size_t n;
  mu_sieve_string_t *s;
  
  if (mach->stringcount == mach->stringmax)
    {
      mu_i_sv_2nrealloc (mach, (void**) &mach->stringspace,
			 &mach->stringmax,
			 sizeof mach->stringspace[0]);
    }

  n = mach->stringcount++;
  s = &mach->stringspace[n];
  memset (s, 0, sizeof *s);
  s->orig = str;

  return n;
}

mu_sieve_string_t *
mu_sieve_string_raw (mu_sieve_machine_t mach, mu_sieve_slice_t slice,
		     size_t i)
{
  if (i >= slice->count)
    abort ();
  return mach->stringspace + slice->first + i;
}

char *
mu_sieve_string_get (mu_sieve_machine_t mach, mu_sieve_string_t *string)
{
  char *exp;
  int rc;
  
  if (string->constant)
    return string->orig;

  rc = mu_i_sv_string_expand (string->orig, mu_i_sv_expand_variables, mach,
			      &exp);
  switch (rc)
    {
    case 0:
      if (string->exp == NULL)
	{
	  string->changed = strcmp (string->orig, exp) != 0;
	  string->exp = mu_sieve_strdup (mach, exp);
	  free (exp);
	}
      else if (strcmp (exp, string->exp) == 0)
	{
	  string->changed = 0;
	  free (exp);
	}
      else
	{
	  string->changed = 1;
	  mu_sieve_free (mach, string->exp);
	  string->exp = mu_sieve_strdup (mach, exp);
	  free (exp);
	}
      break;
	  
    case MU_ERR_CANCELED:
      string->changed = 0;
      return string->orig;

    default:
      mu_sieve_error (mach, "error expanding variables: %s",
		      mu_strerror (rc));
      mu_sieve_abort (mach);
    }

  return string->exp;
}

char *
mu_sieve_string (mu_sieve_machine_t mach, mu_sieve_slice_t slice,
		 size_t i)
{
  return mu_sieve_string_get (mach, mu_sieve_string_raw (mach, slice, i));
}

      
