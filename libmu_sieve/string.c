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

struct mu_sieve_string *
mu_sieve_string_raw (mu_sieve_machine_t mach, mu_sieve_slice_t slice,
		     size_t i)
{
  if (i >= slice->count)
    abort ();
  return mach->stringspace + slice->first + i;
}

char *
mu_sieve_string (mu_sieve_machine_t mach, mu_sieve_slice_t slice,
		 size_t i)
{
  return mu_sieve_string_raw (mach, slice, i)->orig;
}
