/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2002, 2004-2012, 2014-2016 Free Software
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif  

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>  
#include <string.h>  
#include <sieve-priv.h>

void
mu_sieve_require (mu_sieve_machine_t mach, mu_sieve_slice_t list)
{
  size_t i;
  
  for (i = 0; i < list->count; i++)
    {
      struct mu_sieve_string *str = mu_sieve_string_raw (mach, list, i);
      char *name = str->orig;
      int (*reqfn) (mu_sieve_machine_t mach, const char *name) = NULL;
      const char *text = NULL;
      
      if (strncmp (name, "comparator-", 11) == 0)
	{
	  name += 11;
	  reqfn = mu_sieve_require_comparator;
	  text = _("required comparator");
	}
      else if (strncmp (name, "test-", 5)  == 0) /* GNU extension */
	{
	  name += 5;
	  reqfn = mu_sieve_require_test;
	  text = _("required test");
	}
      else if (strcmp (name, "relational") == 0) /* RFC 3431 */
	{
	  reqfn = mu_sieve_require_relational;
	  text = "";
	}
      else if (strcmp (name, "encoded-character") == 0) /* RFC 5228, 2.4.2.4 */
	{
	  reqfn = mu_sieve_require_encoded_character;
	  text = "";
	}
      else
	{
	  reqfn = mu_sieve_require_action;
	  text = _("required action");
	}

      if (reqfn (mach, name))
	{
	  mu_diag_at_locus (MU_LOG_ERROR, &mach->locus,
			    _("source for the %s %s is not available"),
			    text, name);
	  mu_i_sv_error (mach);
	}
    }
}
     
